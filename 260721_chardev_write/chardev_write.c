#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <linux/uaccess.h>




#define DEVICE_NAME "chardev"
#define DEVICE_FILE_NAME "chardev"
#define MAJOR_NUM 100
#define BUF_LEN 128

const char message[BUF_LEN] = "chardev write test\n";
static size_t message_len = sizeof(message) - 1;

static int device_open(struct inode *inode, struct file *file)
{
    pr_info("device open\n");
    return 0;

}

static int device_release(struct inode *inode, struct file *file){


}

static ssize_t device_read(struct file *file, char __user *buf, size_t count, 
loff_t *offset){

    
}

static ssize_t device_write()







