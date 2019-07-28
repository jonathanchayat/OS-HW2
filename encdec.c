#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/string.h>


#include "encdec.h"

#define MODULE_NAME "encdec"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YOUR NAME");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;
char *caesar_buf, *xor_buf;

struct file_operations fops_caesar = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_caesar,
	.write 	 =	encdec_write_caesar,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};

struct file_operations fops_xor = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_xor,
	.write 	 =	encdec_write_xor,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};



/*// Implemetation suggestion:
// -------------------------
// Use this structure as your file-object's private data structure*/
typedef struct {
	unsigned int key;
	int read_state;
} encdec_private_date;


int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{
	    printk("bad dynamic major function init \n");
		return -major;
	}

    caesar_buf= kmalloc(memory_size,GFP_KERNEL);
    xor_buf=kmalloc(memory_size,GFP_KERNEL);
    if(xor_buf==NULL || caesar_buf==NULL)
    {
        printk("bad allocation of the buffers. function init \n");
        return -1;
    }
    memset(caesar_buf,0,memory_size);
    memset(xor_buf,0,memory_size);
/*	// Implemetation suggestion:
	// -------------------------
	// 1. Allocate memory for the two device buffers using kmalloc (each of them should be of size 'memory_size')*/

	return 0;
}

void cleanup_module(void)
{
    unregister_chrdev(major,MODULE_NAME);
    kfree(caesar_buf);
    kfree(xor_buf);
	/*// Implemetation suggestion:
	// -------------------------
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree*/
}

int encdec_open(struct inode *inode, struct file *filp)
{
    encdec_private_date *a;
    if(MINOR(inode->i_rdev)==0)
    {
        filp->f_op= &fops_caesar;
    }
    if(MINOR(inode->i_rdev)==1)
    {
        filp->f_op= &fops_xor;
    }
    if(MINOR(inode->i_rdev)!=0 && MINOR(inode->i_rdev)!= 1)
    {
        printk("minor diffrent from 0 and 1 \n");
        return -ENODEV;
    }

    a= kmalloc(sizeof(encdec_private_date),GFP_KERNEL);
    if(a==NULL)
    {
        printk("bad allocation of a. function open \n");
        return -1;
    }

    a->key=0;
    a->read_state=ENCDEC_READ_STATE_DECRYPT;


    filp->private_data=(encdec_private_date *) kmalloc(sizeof(encdec_private_date),GFP_KERNEL);
    if(filp->private_data==NULL)
    {
        printk("bad allocation of filp->private_data. function a \n");
        return -1;
    }
    filp->private_data= a;
    if(filp->private_data==NULL)
    {
        printk("bad allocation of filp->private_data. function a \n");
        return -1;
    }
    filp->f_pos=0;
/*	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)*/

	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
    kfree(filp->private_data);
/*// Implemetation suggestion:
	// -------------------------
	// 1. Free the allocated memory for 'filp->private_data' (using kfree)*/

	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    encdec_private_date *a = filp->private_data;
    if (a==NULL)
    {
        printk("bad pointing on filp-> private data function ioctl \n");
        return -1;
    }
    switch(cmd)
    {
        case ENCDEC_CMD_CHANGE_KEY:
            a->key= arg;
            break;
        case ENCDEC_CMD_SET_READ_STATE:
            a->read_state=arg;
            break;
        case ENCDEC_CMD_ZERO:
            if(MINOR(inode->i_rdev)==0)
            {
                memset(caesar_buf,0,memory_size);
            }
            if(MINOR(inode->i_rdev)==1)
            {
                memset(xor_buf,0,memory_size);
            }
            filp->f_pos=0;
            break;
        default:
            printk ("bad cmd, function ioctl \n");
            return -1;
    }
    printk("the new key is: %d and the new state read is: %d \n",a->key, a->read_state );
	/*// Implemetation suggestion:
	// -------------------------
	// 1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'*/

	return 0;
}

/*// Add implementations for:
// ------------------------*/
ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    char temp[count];
    int i=0;
    int index= (*f_pos);
    encdec_private_date *device =filp->private_data;
    if(device==NULL)
    {
        printk("bad pointing on filp -> private data function read caesar \n");
        return -1;
    }
    int k = device->key;
    if(index>=memory_size)
        return -EINVAL;
    memset(temp,0,count);

        while(i<count&& index!=memory_size)
        {
            switch(device->read_state)
            {
                case(ENCDEC_READ_STATE_DECRYPT):
                    temp[i]= ((caesar_buf[index] - k)+128) % 128;
                    break;
                case(ENCDEC_READ_STATE_RAW):
                    temp[i]=caesar_buf[index];
                    break;
            }
            i++;
            index++;
        }
        (*f_pos)=index;
        copy_to_user(buf,temp,count);
        printk("the string that the module give to the user buf is: %s \n", temp);
    return 0;
}
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    char temp[count];
    int i=0, n;
    int index= (*f_pos);
    encdec_private_date *device =filp->private_data;
    if(device==NULL)
    {
        printk("bad pointing on filp -> private data function write caesar \n");
        return -1;
    }
    int k = device->key;
    if(index+count >memory_size)
        return -ENOSPC;

    memset(temp,0,count);
    n= copy_from_user(temp,buf,count);
    while(index!=memory_size&&i<count)
    {
        caesar_buf[index]=(temp[i]+k)%128;
        i++;
        index++;
    }
    (*f_pos)=index;
    printk("the string that the module insert to the caesar buffer is: %s \n", caesar_buf);
    return 0;

}
ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    char temp[count];
    int i=0;
    int index= (*f_pos);
    encdec_private_date *device =filp->private_data;
    if(device==NULL)
    {
        printk("bad pointing on filp -> private data. function read xor  \n");
        return -1;
    }
    int k = device->key;
    if(index>=memory_size)
        return -EINVAL;
    memset(temp,0,count);

        while(i<count&& index!=memory_size)
        {
            switch(device->read_state)
            {
                case(ENCDEC_READ_STATE_DECRYPT):
                    temp[i]= xor_buf[index]^k;
                    break;
                case(ENCDEC_READ_STATE_RAW):
                    temp[i]=xor_buf[index];
                    break;
            }
            i++;
            index++;
        }
        (*f_pos)=index;
        copy_to_user(buf,temp,count);
        printk("the string that the module give to the user buf is: %s \n", temp);
    return 0;
}
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    char temp[count];
    int i=0;
    int index= (*f_pos);
    encdec_private_date *device =filp->private_data;
    if(device==NULL)
    {
        printk("bad pointing on filp -> private data function: write xor \n");
        return -1;
    }
    int k = device->key;
    if(index+count>memory_size)
        return -ENOSPC;
    memset(temp,0,count);
    copy_from_user(temp,buf,count);
    while(index!=memory_size&&i<count)
    {
        xor_buf[index]= temp[i]^k;
        i++;
        index++;
    }
    (*f_pos)=index;
    printk("the string that the module give to the xor buffer is: %s \n", temp);
    return 0;
}
