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
	 
	 /* create a pointer to aesd_dev and store pointer held by filp->private_data */
	 struct aesd_dev *dev = filp->private_data;
	 
 	 /* lock mutex */
	 ret_val = mutex_lock_interruptible(&(dev->lock_circular_buffer));
	 if(ret_val != 0)
	 {
	 	// Error in obtaining the mutex. Probably got interrupted
	 	PDEBUG("mutex_lock_interruptible() interrupted\n\r");
	 }
	 
	 /*check count and malloc memory for it */
	 char *temp_buffer = (char *)kmalloc(count, GFP_KERNEL); 
	 if(temp_buffer == NULL)
	 {
	 	PDEBUG("Error in kmalloc for temp_buffer in aesd_read()\n\r");
	 	goto exit_error;
	 }
	 uint32_t temp_buffer_index =0; /* index for temp_buffer */
	 
	 /* Copy that amount of data from the circular buffer and store it in the temp buffer */
	 
	 /* temporary variable that points to the circular buffer entry element containing the last character to be copied */
	 struct aesd_buffer_entry *end_value = NULL; 
	 /* Stores the offset inside the string for the last circular buffer entry element */
	 size_t final_char_offset;
	 dev->char_offset = *f_pos;
	 
	 end_value = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->circ_buff), (*f_pos), &final_char_offset);
	 if(end_value == NULL)
	 {
	 	PDEBUG("Error in aesd_circular_buffer_find_entry_offset_for_fpos() in aesd_read()\n\r");
	 	goto exit_error;
	 }
	 	 
	 	 
 	 //TODO: Check return values. Partial read, End of File, error return values should be possible 
	 uint32_t bytes_copied = 0;
	 int i;
	 for(i =0; i< AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
	 {
	 	/* pointer to element in circular buffer where the read index(out_offs) is */
		 struct aesd_buffer_entry *start_value = &((dev->circ_buff).entry[(dev->circ_buff).out_offs]);
	 	if((start_value) == (end_value))
	 	{
	 		//copy only upto final_char_offset
	 		
	 		/* then only a subset of the string is copied from the element and 
	 		it's possible that some characters might not be read from the element string */
	 		uint32_t j;
	 		if(i ==0) 
	 		{
		 		for( j= dev->char_offset; j<final_char_offset; j++)
		 		{
		 			temp_buffer[temp_buffer_index] = start_value->buffptr[j];
		 			temp_buffer_index++;
		 			bytes_copied++;
		 		}
		 		break;
	 		}
	 		/* Entire remaining string subset is to be copied from the element */
	 		else
	 		{
	 			for(j = 0; j<final_char_offset; j++)
		 		{
		 			temp_buffer[temp_buffer_index] = start_value->buffptr[j];
		 			temp_buffer_index++;
		 			bytes_copied++;
		 		}
		 		break;
	 		} 		
	 	}
	 	else if((start_value) != (end_value))
	 	{
	 		//copy entire string
	 		uint32_t k;
	 		for(k = dev->char_offset; k<(start_value->size); k++)
	 		{
	 			temp_buffer[temp_buffer_index] = start_value->buffptr[k];
	 			temp_buffer_index++;
	 			bytes_copied++;
	 		}
	 		
	 		kfree(start_value->buffptr);
	 		start_value->size =0;
	 		
	 		(dev->circ_buff).out_offs = (((dev->circ_buff).out_offs +1)%(AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED));
	 		dev->char_offset = 0;
	 	}
	 }
	 
	 *f_pos = final_char_offset;
	 
	 
	 /* Copy data from temp buffer to the user space */
	 ret_val = copy_to_user(buf, temp_buffer, bytes_copied);
	 if(ret_val < bytes_copied)
	 {
	 	PDEBUG("Error in copy_to_user() in aesd_read()\n\r");
	 	goto exit_error;
	 }
	 
	 /* Free temp_buffer */
	 kfree(temp_buffer);
	 
	 /* return the number of bytes copied */
	retval = bytes_copied;
	
 exit_error:
	 /* release mutex */
	 mutex_unlock(&(dev->lock_circular_buffer));
	
	 
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	int ret_val;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos); 
	 
 	 /* create a pointer to aesd_dev and store pointer held by filp->private_data */
	 struct aesd_dev *dev = filp->private_data;
	 
  	 /* lock mutex */
 	 ret_val = mutex_lock_interruptible(&(dev->lock_circular_buffer));
	 if(ret_val != 0)
	 {
	 	// Error in obtaining the mutex. Probably got interrupted
	 	PDEBUG("mutex_lock_interruptible() interrupted\n\r");
	 	return -ERESTARTSYS;
	 }
	 
	 PDEBUG("Locked mutex"); 
	 
	 
	 if(dev->temp_write_buffer.size == 0) //size in entry is 0
	 {
	 	/* malloc temp buffer */
		 (dev->temp_write_buffer).buffptr = (char *)kmalloc(count, GFP_KERNEL); 
		 
		 if((dev->temp_write_buffer).buffptr == NULL)
		 {
 		 	PDEBUG("Error in kmalloc for temp_write_buffer in aesd_write()\n\r");
	 		goto exit_error;
		 }
		 //dev->temp_write_buffer.size = count;
		 PDEBUG("Malloced temp_write_buffer"/*, dev->temp_write_buffer.size*/);
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
		 PDEBUG("Realloced temp_write_buffer"/*, dev->temp_write_buffer.size*/);
	 }
	 
	 
	 size_t bytes_write = 0;
	 /* copy from user into global write buffer and check for \n*/
	 bytes_write = copy_from_user((void *)(dev->temp_write_buffer.buffptr + dev->temp_write_buffer.size), buf, count);
	 
	 retval = count-bytes_write;
	 (dev->temp_write_buffer).size += retval;
	 PDEBUG("copy_from_user bytes: %zu", retval);
	 
	 /*int i;
	 for(i = 0; i< dev->temp_write_buffer.size; i++)
	 {
	 	if((dev->temp_write_buffer).buffptr[i] == '\n')
	 	{
	 		dev->string_complete_flag = 1;
	 		break;
	 	}
	 }
	 PDEBUG("Check for \ n");
	 
	 if(dev->string_complete_flag)
	 {
	 	dev->string_complete_flag =0;
	 	void *temp = NULL;
	 	temp = aesd_circular_buffer_add_entry(&(dev->circ_buff), &(dev->temp_write_buffer));
	 	if(temp != NULL)
	 	{
	 		// Incase the buffer is overwriting an entry. Free the memory allocated to the entry previously 
	 		//temp->size =0;
	 		kfree(temp);
	 	}
	 	PDEBUG("Write to circular buffer");
	 	dev->temp_write_buffer.size =0;
	 	dev->temp_write_buffer.buffptr = NULL;
	 }*/
	 
	 if(memchr(dev->temp_write_buffer.buffptr, '\n', dev->temp_write_buffer.size))
	 {
	 	char *temp = NULL;
	 	temp = aesd_circular_buffer_add_entry(&dev->circ_buff, &dev->temp_write_buffer);
	 	if(temp == NULL)
	 	{
	 		kfree(temp);
	 	}
	 	
	 	PDEBUG("Write to circular buffer");
	 	dev->temp_write_buffer.size =0;
	 	dev->temp_write_buffer.buffptr = NULL;
	 }
	 
	 //retval = (dev->temp_write_buffer).size - count;
	
 exit_error:
	 /* release mutex */
	 mutex_unlock(&(dev->lock_circular_buffer));
	 PDEBUG("Released mutex");
	 
	 PDEBUG("Return value %d", retval);
	 
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

	/* TODO: Initialize the circular buffer  Already done by the memset above */
	aesd_circular_buffer_init(&aesd_device.circ_buff); 
	
	/* Initialize the contents of the circular buffer structure */
	aesd_device.circ_buff.in_offs =0;
	aesd_device.circ_buff.out_offs =0;
	aesd_device.circ_buff.full = false;
	
	aesd_device.string_complete_flag =0;
	aesd_device.char_offset =0;
	
	//TODO: Initialize the locking mechanism
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

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	 
	 // free memory allocated to temp_write_buffer
	 kfree(aesd_device.temp_write_buffer.buffptr);
	 
	 //TODO: Uninit locking mechanism? Check if some process is still holding the mutex and send an error message?

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
