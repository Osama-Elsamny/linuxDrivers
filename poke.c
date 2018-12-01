#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

#define  DEVICE_NAME "poke"
#define  CLASS_NAME  "pokeclass"
static DEFINE_MUTEX(pokeMutex);

static struct device* pokeDevice = NULL;
static struct class* pokeClass = NULL;
static int majorNumber = 0;

static int poke_open(struct inode *pinode, struct file *pfile) {
    printk(KERN_ALERT "Inside the poke_open function\n");
    // Try to acquire the mutex
    if(!mutex_trylock(&pokeMutex)) {
	    printk(KERN_ALERT "poke_open: Device in use by another process");
	    return -EBUSY;
    }
    return 0;
}
static ssize_t poke_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) {
    printk(KERN_ALERT "Inside the poke_read function\n");
    printk(KERN_INFO "poke_read: There is no read function for poke\n");
    return -EFAULT;
}
static ssize_t poke_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) {
    printk(KERN_ALERT "Inside the poke_write function\n");
    
    if(!copy_from_user(address, buffer, length)) {
        printk(KERN_INFO "peek_read: Sent one byte to the user: %c\n", *ptr);
        return 0;
    }else {
        printk(KERN_INFO "peek_read: Failed to send one byte to the user\n");
        return -EFAULT; 
    }
}
static int poke_close(struct inode *pinode, struct file *pfile) {
    // Release the mutex
    mutex_unlock(&pokeMutex);
    printk(KERN_ALERT "Inside the poke_close function\n");
    return 0;
}

/**
 * The file operation performed on the device
 * The file_operation sturcture is defined in /linux/fs.h
 * */ 
struct file_operations poke_file_operation = {
    .open = poke_open,
    .read = poke_read,
    .write = poke_write,
    .release = poke_close,
};

static int __init poke_module_init(void) {
    printk(KERN_ALERT "Inside the poke_module_init function\n");
    //Registering the Character devie with the kernel
    // function parameters : Major Number, Name of the Driver, File operatons
    majorNumber = register_chrdev(0, DEVICE_NAME, &poke_file_operation);
    if(majorNumber < 0) {
        printk(KERN_ALERT "poke_module_init: failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "poke_module_init: registered correctly with major number %d\n", majorNumber);
    // Registering a class as shown by Karol
    pokeClass = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(pokeClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "poke_module_init: Failed to register device class\n");
        return PTR_ERR(pokeClass);
    }
    printk(KERN_INFO "poke_module_init: device class registered correctly\n");
    //Registering the device driver
    pokeDevice = device_create(pokeClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if(IS_ERR(pokeDevice)) {
        class_destroy(pokeClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "poke_module_init: Failed to create the device\n");
        return PTR_ERR(pokeDevice);
    }
    printk(KERN_INFO "poke_module_init: device class created correctly\n");
    mutex_init(&pokeMutex); 
    return 0;
}
static void __exit poke_module_exit(void) {
    printk(KERN_ALERT "Inside the poke_module_exit function\n");
    //Unregistering the Character devie with the kernel
    // Reversing init
    mutex_destroy(&pokeMutex);
    device_destroy(pokeClass, MKDEV(majorNumber, 0));
    class_unregister(pokeClass);
    class_destroy(pokeClass); 
    unregister_chrdev(majorNumber, DEVICE_NAME); 
    printk(KERN_INFO "poke_module_exit: Goodbye!\n");
}
module_init(poke_module_init);
module_exit(poke_module_exit);