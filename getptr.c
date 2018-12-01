#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#define DEVICE_NAME "getptr"
#define CLASS_NAME "getptrclass"
#define BUFFER_SIZE 1024
#define ADDRESS_SIZE 8 
static DEFINE_MUTEX(getptrMutex);

MODULE_LICENSE("GPL");

static int majorNumber = 0;
char* array;
static struct device* getptrDevice = NULL;
static struct class* getptrClass = NULL;

static int getptr_open(struct inode *pinode, struct file *pfile) {
    printk(KERN_ALERT "Inside the getptr_open function\n");
    // Try to acquire the mutex
    if(!mutex_trylock(&getptrMutex)) {
	    printk(KERN_ALERT "getptr_open: Device in use by another process");
	    return -EBUSY;
    }
    array = (char*) kmalloc(BUFFER_SIZE * sizeof(char), GFP_KERNEL);
    return 0;
}
static ssize_t getptr_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) {
    char temp[ADDRESS_SIZE] = {0};
    printk(KERN_ALERT "Inside the getptr_read function\n");
    if(length != ADDRESS_SIZE) {
        printk(KERN_INFO "getptr_read: Failed the length of the buffer should be 8 bytes\n");
        return -EFAULT; 
    }
    memcpy(temp, &array, sizeof(char*));
    if(!copy_to_user(buffer, temp, ADDRESS_SIZE)) {
        printk(KERN_INFO "getptr_read: Sent the address: %p to the user\n", array);
        return 0;
    }else {
        printk(KERN_INFO "getptr_read: Failed to send the address to the user\n");
        return -EFAULT; 
    }
}
static ssize_t getptr_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) {
    printk(KERN_ALERT "Inside the getptr_write function\n");
    printk(KERN_INFO "getptr_write: There is no write function for getptr\n");
    return -EFAULT;
}
static int getptr_close(struct inode *pinode, struct file *pfile) {
    // Release the mutex
    kfree(array);
    mutex_unlock(&getptrMutex);
    printk(KERN_ALERT "Inside the getptr_close function\n");
    return 0;
}

/**
 * The file operation performed on the device
 * The file_operation sturcture is defined in /linux/fs.h
 * */ 
struct file_operations getptr_file_operation = {
    .open = getptr_open,
    .read = getptr_read,
    .write = getptr_write,
    .release = getptr_close,
};

static int __init getptr_module_init(void) {
    printk(KERN_ALERT "Inside the getptr_module_init function\n");
    //Registering the Character devie with the kernel
    // function parameters : Major Number, Name of the Driver, File operatons
    majorNumber = register_chrdev(0, DEVICE_NAME, &getptr_file_operation);
    if(majorNumber < 0) {
        printk(KERN_ALERT "getptr_module_init: failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "getptr_module_init: registered correctly with major number %d\n", majorNumber);
    // Registering a class as shown by Karol
    getptrClass = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(getptrClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "getptr_module_init: Failed to register device class\n");
        return PTR_ERR(getptrClass);
    }
    printk(KERN_INFO "getptr_module_init: device class registered correctly\n");
    //Registering the device driver
    getptrDevice = device_create(getptrClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if(IS_ERR(getptrDevice)) {
        class_destroy(getptrClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "getptr_module_init: Failed to create the device\n");
        return PTR_ERR(getptrDevice);
    }
    printk(KERN_INFO "getptr_module_init: device class created correctly\n");
    mutex_init(&getptrMutex); 
    return 0;
}
static void __exit getptr_module_exit(void) {
    printk(KERN_ALERT "Inside the getptr_module_exit function\n");
    //Unregistering the Character devie with the kernel
    // Reversing init
    mutex_destroy(&getptrMutex);
    device_destroy(getptrClass, MKDEV(majorNumber, 0));
    class_unregister(getptrClass);
    class_destroy(getptrClass); 
    unregister_chrdev(majorNumber, DEVICE_NAME); 
    printk(KERN_INFO "getptr_module_exit: Goodbye!\n");
}
module_init(getptr_module_init);
module_exit(getptr_module_exit);