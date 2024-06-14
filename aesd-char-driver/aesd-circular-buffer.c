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



struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */ 
    size_t boffset = 0;
    int i, count;
    for (i = buffer->out_offs, count = 0; count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED, count ++){
        if(char_offset < boffset + buffer->entry[i].size){
            *entry_offset_byte_rtn = char_offset - boffset;
            return &buffer->entry[i];
        }
        boffset += buffer->entry[i].size;
    }
    return NULL;
}


void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{ 
    /**
    * TODO: implement per description
    */
    buffer->entry[buffer->in_offs] = *add_entry;
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    if(buffer->full){
        buffer->out_offs = (buffer->out_offs +1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    buffer->full = (buffer->in_offs == buffer->out_offs);
}

void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer){

    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
