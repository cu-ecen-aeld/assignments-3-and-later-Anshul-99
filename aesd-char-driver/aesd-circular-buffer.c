/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"
//#include "syslog.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{

	uint8_t temp_index = (buffer->out_offs);
	size_t temp = 0;
	size_t temp2 = char_offset;
	
	if(buffer->full)
	{
		int i;
		for(i = 0; i<AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
		{
			temp =  ((buffer->entry[temp_index]).size);
			
			if(temp > temp2)
			{
				//syslog(LOG_DEBUG, "Temp_: %ld\n", temp);
				*(entry_offset_byte_rtn) = temp2;
				return &(buffer->entry[temp_index]);
			}
			/*else if(temp == temp2)
			{
				syslog(LOG_DEBUG, "Temp2_: %ld\n", temp2);
				*(entry_offset_byte_rtn) = temp2 - temp;
				temp_index = ((temp_index +1)%(AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED));
				return &(buffer->entry[temp_index]);
			}*/
			else
			{
				temp_index = ((temp_index +1)%(AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED));
				temp2 -= temp;
			}
		}
	}
	else
	{
		/* When buffer isn't full */
		while(temp_index != (buffer->in_offs))
		{
			temp =  ((buffer->entry[temp_index]).size);
			if(temp >= temp2)
			{
				*(entry_offset_byte_rtn) = temp2; /*(((buffer->entry[temp_index]).buffptr)[temp2]);*/
				return &(buffer->entry[temp_index]);
			}
			else
			{
				temp_index = ((temp_index +1)%(AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED));
				temp2 -= temp;
			}
			
		}
	}
	
	//syslog(LOG_DEBUG, "***********************************\n");
	/* In case of error */
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char *aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{  
    /* Buffer full condition */
    if(buffer->full)
    {
    	buffer->out_offs = ((buffer->out_offs +1)%(AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED));
    	buffer->full = false; //TODO
    }
    
    const char *temp = NULL;
    temp = (buffer->entry[buffer->in_offs]).buffptr;
    
    /* Store pointer and size in the circular buffer */
    (buffer->entry[buffer->in_offs]).buffptr = add_entry->buffptr;
    (buffer->entry[buffer->in_offs]).size = add_entry->size;
    
    /*Check if buffer is full */
    buffer->in_offs = ((buffer->in_offs+1)%(AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED));
    if(buffer->in_offs == buffer->out_offs)
    {
    	buffer->full = true;
    }
    return temp;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
