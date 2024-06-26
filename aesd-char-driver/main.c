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
#include "aesd_ioctl.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Andrei Zargarov"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp){
    struct aesd_dev * dev;
    dev =  container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;

    PDEBUG("Device opened\n");
   
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp){
    //struct aesd_dev * dev = filp->private_data;
    //uint8_t index;
    //struct aesd_circular_buffer buffer = dev->circular_buffer;
    //struct aesd_buffer_entry *entry;

    PDEBUG("release");
    /**
     * TODO: handle release
     */
    //Example usage:
    
    //AESD_CIRCULAR_BUFFER_FOREACH(entry,&buffer,index) {
    //     kfree(entry->buffptr);
    //}
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev * dev = filp->private_data;
    struct aesd_buffer_entry *buffer;
    size_t offset;
    ssize_t retval = 0;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    mutex_lock(&dev->mtx);
    buffer = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &offset);
    if (buffer == NULL){
        mutex_unlock(&dev->mtx);
        return retval;
    }

    if (count < (buffer->size - offset)){
        retval = count;
    }else{
        retval = buffer->size - offset;
    }
    PDEBUG("RETVAL : %zd", retval);
    if (copy_to_user(buf, buffer->buffptr + offset, retval) !=0){
        mutex_unlock(&dev->mtx);
        return -EFAULT;
    }
    *f_pos = *f_pos + retval;
    mutex_unlock(&dev->mtx);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
    
    struct aesd_dev * dev = filp->private_data;
    ssize_t retval = -ENOMEM;
    ssize_t remain = 0;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    mutex_lock(&dev->mtx);
    if (dev->entry.size == 0){
        dev->entry.buffptr = kzalloc(count, GFP_KERNEL);
        PDEBUG("Allocating memory for %zd", count);
    }else{
        PDEBUG("re Allocating memory for %zd", count);
        dev->entry.buffptr = krealloc(dev->entry.buffptr, dev->entry.size + count, GFP_KERNEL);
    }
    if(dev->entry.buffptr == NULL){
        mutex_unlock(&dev->mtx);
        return retval;
    }
    remain = copy_from_user((void*) &dev->entry.buffptr[dev->entry.size], buf, count);
    PDEBUG("Copied from user buffer to memory, reminded %zd", remain);
    if (remain > 0){
        retval = count - remain;
    }else{
        retval = count;
    }
    dev->entry.size +=  retval;

    if(strchr((char *) dev->entry.buffptr, '\n')){
        aesd_circular_buffer_add_entry(&dev->circular_buffer, &dev->entry);
        dev->entry.buffptr = NULL;
        dev->entry.size = 0;
    }
    mutex_unlock(&dev->mtx);
    return retval;
}



loff_t aesd_llseek(struct file *file, loff_t offset, int whence) {
    struct aesd_dev *dev = file->private_data;
    struct aesd_circular_buffer *buffer = &dev->circular_buffer;
    loff_t new_pos = 0;
    size_t total_size = 0;
    int i, count;

    mutex_lock(&dev->mtx);

    // Calculate total size of the circular buffer content
    for (i = buffer->out_offs, count = 0; count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED, count++) {
        total_size += buffer->entry[i].size;
        if (i == buffer->in_offs && !buffer->full)
            break;
    }

    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = total_size + offset;
            break;
        default:
            mutex_unlock(&dev->mtx);
            return -EINVAL;
    }

    if (new_pos < 0 || new_pos > total_size) {
        mutex_unlock(&dev->mtx);
        return -EINVAL;
    }

    file->f_pos = new_pos;
    mutex_unlock(&dev->mtx);
    return new_pos;
}

static long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;
    struct aesd_circular_buffer *buffer = &dev->circular_buffer;
    int i, count;
    size_t new_pos = 0;

    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;

    switch (cmd) {
    case AESDCHAR_IOCSEEKTO:
        if (copy_from_user(&seekto, (struct aesd_seekto __user *)arg, sizeof(seekto)))
            return -EFAULT;

        mutex_lock(&dev->mtx);

        // Calculate total size of the circular buffer content
        for (i = buffer->out_offs, count = 0; count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED, count++) {
            if (i == buffer->in_offs && !buffer->full) break;
        }

        if (seekto.write_cmd >= count || buffer->entry[seekto.write_cmd].size <= seekto.write_cmd_offset) {
            mutex_unlock(&dev->mtx);
            return -EINVAL;
        }

        for (i = buffer->out_offs, count = 0; count < seekto.write_cmd; i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED, count++) {
            new_pos += buffer->entry[i].size;
            if (i == buffer->in_offs && !buffer->full) break;
        }

        new_pos += seekto.write_cmd_offset;
        filp->f_pos = new_pos;

        mutex_unlock(&dev->mtx);
        break;

    default:
        return -ENOTTY;
    }

    return 0;    
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek = aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
};


static int aesd_setup_cdev(struct aesd_dev *dev){
    int err, devno = MKDEV(aesd_major, aesd_minor);
    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }

    mutex_init(&dev->mtx);
    return err;
}

int aesd_init_module(void){
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

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void){
    dev_t devno = MKDEV(aesd_major, aesd_minor); 
    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
