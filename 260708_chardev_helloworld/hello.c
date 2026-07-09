#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

int init_module(void){
    printk(KERN_INFO "hello world\n" );
    return 0;
}

void cleanup_module(void){
    printk(KERN_INFO "GOODBYE\n");

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yoochan");
MODULE_DESCRIPTION("Simple hello world kernel module");