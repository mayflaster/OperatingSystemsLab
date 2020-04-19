/* my_module.c: Example char device module.
 *
 */
/* Kernel Programming */
#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/kernel.h>  	
#include <linux/module.h>
#include <linux/fs.h>       		
#include <asm/uaccess.h>
#include <linux/errno.h>  
#include <linux/slab.h>
#include <asm/segment.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>


#include "my_module.h"

#define MY_DEVICE "s20_device"
#define MinorsNum 256
#define SizeOfPicX 220
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anonymous");

/* globals */
int my_major = 0; /* will hold the major # of my device driver */

struct file_operations my_fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .ioctl = my_ioctl
};

typedef struct Data_{
    int offset;
   // int write_x;
    //int write_y;
    int minor;
   // char buff[SizeOfPicX][SizeOfPicX];
    //int Counter;
} Data;

typedef struct file_ {
	char buffer [SizeOfPicX*SizeOfPicX];
	int open;
} File; 

File files [MinorsNum];

//char files [MinorsNum][SizeOfPicX*SizeOfPicX]; // store the device files of this driver
int init_module(void)
{
    my_major = register_chrdev(my_major, MY_DEVICE, &my_fops);

    if (my_major < 0)
    {
	printk(KERN_WARNING "can't get dynamic major\n");
	return my_major;
    }
	int i;
    for ( i=0; i < MinorsNum; i++){
    files[i].open=0;
    }
    return 0;
}


void cleanup_module(void)
{
    unregister_chrdev(my_major, MY_DEVICE);

    //
    // do clean_up();

    //
    return;
}


int my_open(struct inode *inode, struct file *filp)
{
    int minor = MINOR(inode->i_rdev);// handle open
    (Data*)(filp-> private_data)=(Data*)vmalloc(sizeof(Data));
	if((Data*)(filp-> private_data) == NULL) 
		return -EFAULT;
    ((Data*)(filp-> private_data))->minor = minor;
    ((Data*)(filp-> private_data))->offset = 0;
	filp->f_pos = 0;
if(files[minor].open ==0) {
	int i; 
    for ( i = 0; i < SizeOfPicX*SizeOfPicX; i++) {

            files[minor].buffer[i] = '\0';
			
			
    }
	//printk("init buffer done\n");
	files[minor].open=1;
}
//printk("open done\n");
    return 0;
}


int my_release(struct inode *inode, struct file *filp)
{
    //int minor = MINOR(inode->i_rdev);
	if (!filp->private_data)
		return -EFAULT;
    vfree(filp->private_data);
	//printk("release done\n");
    //files[minor]=NULL;
    //continue
    return 0;
}

ssize_t my_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    //
    // Do read operation.
    // Return number of bytes read.
	int minor = ((Data *)filp->private_data)->minor;
	int offset = ((Data *)filp->private_data)->offset;
	offset = *f_pos;
	//int offset = f_pos;
	//printk("f pos %d \n",f_pos);
    if((SizeOfPicX*SizeOfPicX) - offset < count){
        return -EACCES;
    }
   int charLeftToBeCopied = copy_to_user (	 buf,files[minor].buffer+offset,count);
    if(charLeftToBeCopied!=0)
        return -EBADF;
	*f_pos+=count;
    ((Data*)(filp->private_data))->offset +=count;
	//printk("read done count %d\n", count);
    return count;
}



struct file *file_open(const char *path)
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, O_RDWR, O_APPEND);
    set_fs(oldfs);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void file_close(struct file *file)
{
    filp_close(file, NULL);
}


int file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = kernel_read(file, offset, data, size);

    set_fs(oldfs);
    return ret;
}


int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int minor = MINOR(inode->i_rdev);
	int i;
	struct file* file_;
	int readNum;
	long len = strlen_user((char*)arg);
	char * path;
    switch(cmd)
    {
    case RESET:
	//

	// handle
	
	for (i = 0; i < SizeOfPicX*SizeOfPicX; i++) {
                    files[minor].buffer[i] = '0';
              
	}
		//printk("reset done\n");
	//
	break;
    case CAPTURE:
		path = (char*) vmalloc (len);
		copy_from_user(path,(char*)arg,len);
         file_=file_open((char*)path);
		 if(!file_){
			 vfree(path);
			 return -EFAULT;
		 }
		 vfree(path);
         readNum= file_read(file_,0,files[minor].buffer,SizeOfPicX*SizeOfPicX);
    if (readNum!= SizeOfPicX*SizeOfPicX)
    return -EFAULT;
    file_close(file_);
	//printk("capture\n");
	break;

    default:
	return -ENOTTY;
    }

    return 0;
}
