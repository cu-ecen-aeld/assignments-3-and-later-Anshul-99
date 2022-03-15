/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"

#include <linux/slab.h>

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Anshul Somani"); /** fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	PDEBUG("open");
	 
	 struct aesd_dev *dev;
	 dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	 filp->private_data = dev;
	 
	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	 
	 //clean up whatever was done on open()
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	int ret_val;
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	
	/******************************************************
	
		Check for invalid arguments
	
	******************************************************/
	
	 if(count == 0)
	 {
	 	return 0;
	 }
	 
	 if((filp == NULL) || (f_pos == NULL) || (buf == NULL))
	 {
	 	return -EFAULT;
	 }
	 
	 
 	/******************************************************
	
		create a pointer to aesd_dev and store pointer held by filp->private_data
	
	******************************************************/
	 struct aesd_dev *dev = filp->private_data;
	 
	 /******************************************************
	 
 	  	lock mutex 
 	  
 	  ******************************************************/
	 ret_val = mutex_lock_interruptible(&(dev->lock_circular_buffer));
	 if(ret_val != 0)
	 {
	 	// Error in obtaining the mutex. Probably got interrupted
	 	PDEBUG("mutex_lock_interruptible() interrupted\n\r");
	 	return -ERESTARTSYS;
	 }
	 PDEBUG("aesd_read: Locked mutex"); 
	 
	 
	 /******************************************************
	 
 	  	Get entry offset for f_pos 
 	  
 	  ******************************************************/		 
 	 /* Pointer to struct entry to be read */
	 struct aesd_buffer_entry *end_value = NULL; 
	 /* Stores the f_pos from where the string needs to be read */
	 size_t final_f_pos;

	 
	 end_value = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->circ_buff), (*f_pos), &final_f_pos);
	 if(end_value == NULL)
	 {
	 	PDEBUG("Error in aesd_circular_buffer_find_entry_offset_for_fpos() in aesd_read()\n\r");
	 	goto exit_error;
	 }
	 
	 PDEBUG("aesd_read: Calculated fpos"); 
	 	 
	 	 
 	 /******************************************************
	 
 	  	Copies string from kernel space to user space 
 	  
 	  ******************************************************/
	 size_t bytes_copied = 0;

	 if( count> (end_value->size - final_f_pos))
	 {
	 	count = end_value->size - final_f_pos;
	 }

	bytes_copied = copy_to_user(buf, (end_value->buffptr + final_f_pos), count);
	if(bytes_copied != 0)
	{
		PDEBUG("Error in copy_to_user() in aesd_read()\n\r");
		goto exit_error;
	}
	PDEBUG("aesd_read: copy to user bytes: %d", bytes_copied); 
	 
 	 /******************************************************
	 
 	  	Update f_pos and calculate bytes copied 
 	  
 	 ******************************************************/
	retval = count - bytes_copied;
	(*f_pos) += retval;
	
	
	 /******************************************************
	 
 	  	Release mutex 
 	  
 	  ******************************************************/
 exit_error:
	 /* release mutex */
	 mutex_unlock(&(dev->lock_circular_buffer));
 	 PDEBUG("aesd_read: Released mutex");
	 
	 PDEBUG("aesd_read: Return value %d", retval);
	
	 
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	int ret_val;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	
	/******************************************************
	
		Check for invalid arguments
	
	******************************************************/
	
	 if(count == 0)
	 {
	 	return 0;
	 }
	 
	 if((filp == NULL) || (f_pos == NULL) || (buf == NULL))
	 {
	 	return -EFAULT;
	 }
	 
 	/******************************************************
	
		create a pointer to aesd_dev and store pointer held by filp->private_data
	
	******************************************************/
	 struct aesd_dev *dev = filp->private_data;
	 
	 /******************************************************
	 
 	  	lock mutex 
 	  
 	  ******************************************************/
 	 ret_val = mutex_lock_interruptible(&(dev->lock_circular_buffer));
	 if(ret_val != 0)
	 {
	 	// Error in obtaining the mutex. Probably got interrupted
	 	PDEBUG("aesd_write: mutex_lock_interruptible() interrupted\n\r");
	 	return -ERESTARTSYS;
	 }
	 
	 PDEBUG("aesd_write: Locked mutex"); 
	 
 	/******************************************************
	
		Write to the circular buffer if \n character is present in the string
		Otherwise, store it in the global string. 
	
	******************************************************/	 
	 if(dev->temp_write_buffer.size == 0) //size in entry is 0
	 {
	 	/* malloc temp buffer */
		 (dev->temp_write_buffer).buffptr = (char *)kmalloc(count, GFP_KERNEL); 
		 dev->malloc_cntr += 1;
		 
		 if((dev->temp_write_buffer).buffptr == NULL)
		 {
 		 	PDEBUG("Error in kmalloc for temp_write_buffer in aesd_write()\n\r");
	 		goto exit_error;
		 }
		 //dev->temp_write_buffer.size = count;
		 PDEBUG("aesd_write: Malloced temp_write_buffer bytes: %d", count/*, dev->temp_write_buffer.size*/);
	 }
	 else 
	 {
	 	/* realloc temp buffer */
	 	(dev->temp_write_buffer).buffptr = krealloc((dev->temp_write_buffer).buffptr, (sizeof(char) *(count + (dev->temp_write_buffer).size)), GFP_KERNEL);
	 	
		 if(dev->temp_write_buffer.buffptr == NULL)
		 {
 		 	PDEBUG("Error in krealloc for temp_write_buffer in aesd_write()\n\r");
	 		goto exit_error;
		 }
		 //dev->temp_write_buffer.size += count;
		 PDEBUG("aesd_write: Realloced temp_write_buffer total size: %d", (count + (dev->temp_write_buffer).size)/*, dev->temp_write_buffer.size*/);
	 }
	 
	 
  	 /******************************************************
	 
 	  	Copies string from user space to kernel space 
 	  
 	 ******************************************************/
	 size_t bytes_write = 0;
	 bytes_write = copy_from_user((void *)(dev->temp_write_buffer.buffptr + dev->temp_write_buffer.size), buf, count);
	 
	 retval = count-bytes_write;
	 (dev->temp_write_buffer).size += retval;
	 PDEBUG("aesd_write: copy_from_user bytes: %zu", retval);
	 
	 int i = 0;
	 for(i = 0; i< dev->temp_write_buffer.size; i++)
	 {
 		 if(dev->temp_write_buffer.buffptr[i] == '\n')
		 {
		 	char *temp = NULL;
		 	temp = aesd_circular_buffer_add_entry(&dev->circ_buff, &dev->temp_write_buffer);
		 	if(temp != NULL)
		 	{
		 		kfree(temp);
		 		dev->malloc_cntr -= 1;
		 	}
		 	
		 	PDEBUG("aesd_write: Write to circular buffer %s", dev->temp_write_buffer.buffptr);
		 	
		 	int i;
		 	for(i =0; i< AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
		 	{
		 		PDEBUG("aesd_write: circular buffer content %d %s", i, dev->circ_buff.entry[i].buffptr);
		 	}
		 	dev->temp_write_buffer.size =0;
		 	dev->temp_write_buffer.buffptr = NULL;
		 	PDEBUG("aesd_write: malloc_cntr %d", dev->malloc_cntr);
		 	break;
		 }
	 }
	 //andjabvkd
	 
	 /*if(memchr(dev->temp_write_buffer.buffptr, '\n', dev->temp_write_buffer.size))
	 {
	 	char *temp = NULL;
	 	temp = aesd_circular_buffer_add_entry(&dev->circ_buff, &dev->temp_write_buffer);
	 	if(temp != NULL)
	 	{
	 		kfree(temp);
	 		dev->malloc_cntr -= 1;
	 	}
	 	
	 	PDEBUG("aesd_write: Write to circular buffer %s", dev->temp_write_buffer.buffptr);
	 	
	 	int i;
	 	for(i =0; i< AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
	 	{
	 		PDEBUG("aesd_write: circular buffer content %d %s", i, dev->circ_buff.entry[i].buffptr);
	 	}
	 	dev->temp_write_buffer.size =0;
	 	dev->temp_write_buffer.buffptr = NULL;
	 	PDEBUG("aesd_write: malloc_cntr %d", dev->malloc_cntr);
	 }*/
	 

	 
 	 /******************************************************
	 
 	  	Release mutex 
 	  
 	 ******************************************************/
	
 exit_error:
	 mutex_unlock(&(dev->lock_circular_buffer));
	 PDEBUG("aesd_write: Released mutex");
	 
	 PDEBUG("aesd_write: Return value %d", retval);
	 
	return retval;
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/* Initialize the circular buffer  Already done by the memset above */
	aesd_circular_buffer_init(&aesd_device.circ_buff); 
	
	/* Initialize the contents of the circular buffer structure */
	aesd_device.circ_buff.in_offs =0;
	aesd_device.circ_buff.out_offs =0;
	aesd_device.circ_buff.full = false;
	
	aesd_device.malloc_cntr = 0;
	
	// Initialize the locking mechanism
	mutex_init(&(aesd_device.lock_circular_buffer));

	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);
	 
	 // free memory allocated to temp_write_buffer
	 kfree(aesd_device.temp_write_buffer.buffptr);

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
