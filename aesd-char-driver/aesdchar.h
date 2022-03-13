/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#include "aesd-circular-buffer.h"

struct aesd_dev
{
	/**
	 * TODO: Add structure(s) and locks needed to complete assignment requirements
	 */
	 
	/* Struct contains the circular buffer and related flags that will be used 
	by the device driver */ 
	struct aesd_circular_buffer circ_buff; 
	
	/* struct aesd_buffer_entry type variable that acts as a buffer for writes. 
	 * In case of writes the values are appended into this buffer until '/n' is receieved */
	 struct aesd_buffer_entry temp_write_buffer;
	 /*Contains the no. of bytes allocated to the string in temp_write_buffer */
	 uint32_t size_temp_write_buffer; 
	 uint8_t string_complete_flag;
	 
	 size_t char_offset;
	
	
	/*Locking mechanism for the accessing the circular buffer via the driver */
	//TODO: spinlock or mutex?
	/* This mutex locks access to circular buffer for read/write operations */
	struct mutex lock_circular_buffer;
	//DECLARE_MUTEX(lock_circular_buffer); 


	/* Char device structure		*/
	struct cdev cdev;	  
};


#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
