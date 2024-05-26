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
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Andrei Zargarov"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp){
    //struct aesd_dev * aesd_ctx;

    PDEBUG("aesd: Device opened\n");
    ///**
    // * TODO: handle open
    // */
    //aesd_ctx = kmalloc(sizeof(struct aesd_dev), GFP_KERNEL);
    //if(! aesd_ctx){
    //    PDEBUG("aesd: Failed to allocate memory for device context");
    //    return -ENOMEM;
    //}
    ////TO DO: initalize aesd_dev struct
    //filp->private_data = aesd_ctx;
    //aesd_ctx->circular_buffer = kmalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);
    //aesd_circular_buffer_init(aesd_ctx->circular_buffer);
    PDEBUG("Open function, allocated context struct and circular buffer\n");
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp){
    struct aesd_dev * aesd_ctx;
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    // release circular buffer
    aesd_ctx = (struct aesd_dev *) filp->private_data;
    kfree(aesd_ctx->circular_buffer);
    kfree(aesd_ctx);
    PDEBUG("release function: released circular buffer and context struct\n");
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */

    //struct aesd_circular_buffer * aesd_buffer = (aesd_circular_buffer *) filp->private_data

    //struct aesd_buffer_entry * buffer_entry =  aesd_circular_buffer_find_entry_offset_for_fpos(aesd_buffer, size_t char_offset, size_t *entry_offset_byte_rtn );

    PDEBUG("read function is not ready yet\n");
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
    //char * kernel_buffer;
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    //kernel_buffer = kmalloc(count, GFP_KERNEL);
    //if (! kernel_buffer){
    //    goto out;
    //}
    //if (copy_from_user(kernel_buffer, buf, count)){
    //    goto out_free;
    //}
    //
    //// TODO: add datat to circular buffer
    //retval = count;
//
    //out_free:
    //    kfree(kernel_buffer);
    //out:
    //PDEBUG("Writeen: %s \n", buf );
    PDEBUG("write is not completed yet\n");

    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev){
    int err, devno = MKDEV(aesd_major, aesd_minor);
    PDEBUG("Setup function is not ready yet\n");

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    PDEBUG("aesd setup cdev function is not ready yet\n");
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
    
    // circular buffer initialization
    aesd_device.circular_buffer = kmalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);
    aesd_circular_buffer_init(aesd_device.circular_buffer);


    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    PDEBUG("Init function is not ready yet\n");
    return result;

}

void aesd_cleanup_module(void){
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    kfree(aesd_device.circular_buffer);

    unregister_chrdev_region(devno, 1);
    PDEBUG("Clean up function is not ready yet\n");
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);