#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

#define DEVICE_NAME "peek"
#define CLASS_NAME  "peekclass"
#define MEMORY_SIZE 1000
#define COMPLETE_WRITE_PEEK_SIZE 8
static DEFINE_MUTEX(peekMutex);

static char writeData[MEMORY_SIZE];
static int index = 0;
static int size = 0;
static struct device* peekDevice = NULL;
static struct class* peekClass = NULL;
static int majorNumber = 0;

static int peek_open(struct inode *pinode, struct file *pfile) {
    printk(KERN_ALERT "Inside the peek_open function\n");
    // Try to acquire the mutex
    if(!mutex_trylock(&peekMutex)) {
	    printk(KERN_ALERT "peek_open: Device in use by another process");
	    return -EBUSY;
    }
    return 0;
}
static ssize_t peek_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) {
    static char *ptr;
    printk(KERN_ALERT "Inside the peek_read function\n");
    if(COMPLETE_WRITE_PEEK_SIZE >= (size - index)){
        memcpy(&ptr, writeData + index, COMPLETE_WRITE_PEEK_SIZE);
        index += COMPLETE_WRITE_PEEK_SIZE;
        if(!copy_to_user(buffer, ptr, 1)) {
            printk(KERN_INFO "peek_read: Sent one byte to the user: %c\n", *ptr);
            return 0;
        }else {
            printk(KERN_INFO "peek_read: Failed to send one byte to the user\n");
            return -EFAULT; 
        }
    }else{
        printk(KERN_INFO "peek_read: Failed \"Not enough bytes in memory to be read, need at least 8 bytes to perform a read\"\n");
        return -EFAULT; 
    }
    return 0;
}
static ssize_t peek_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) { 
    static char address[MEMORY_SIZE];
    static void *ptr;
    static int i = 0;
    printk(KERN_ALERT "Inside the peek_write function\n");
    if(((size + length) > MEMORY_SIZE) && (index != 0)) {
        printk(KERN_INFO "peek_write: Rearranging the memory\n");
        size = size - index;
        memmove(writeData, writeData + index, size);
        //can be removed
        for(i = 0; i < size; i++){
		    printk("writeData: %2.2x", (unsigned int)(unsigned char)writeData[i]);            
        }
        //end of the block to be removeed
    }
    if(((size + length) > MEMORY_SIZE) && (index == 0)) {
        printk(KERN_INFO "peek_write: Failed the maximum number of bytes to be save is %d\n", MEMORY_SIZE);
        return -EFAULT;
    }
    if(!copy_from_user(address, buffer, length)) {
        //can be removed
        memcpy(&ptr, address, length);
        //end of the block to be removeed
        printk(KERN_INFO "peek_write: Pointer is added the data to memeory successfully with address: %p\n", ptr); 
        memmove(writeData + size, address, length);
        size += length;
        //can be removed
        for(i = 0; i < length; i++){
		    printk("address: %2.2x", (unsigned int)(unsigned char)address[i]);            
        }
        for(i = 0; i < size; i++){
		    printk("writeData: %2.2x", (unsigned int)(unsigned char)writeData[i]);            
        }
        //end of the block to be removeed
        return length;
    }else {
        printk(KERN_INFO "peek_write: Failed get the address from the user\n");
        return -EFAULT; 
    }
}
static int peek_close(struct inode *pinode, struct file *pfile) {
    // Release the mutex
    mutex_unlock(&peekMutex);
    printk(KERN_ALERT "Inside the peek_close function\n");
    return 0;
}

/**
 * The file operation performed on the device
 * The file_operation sturcture is defined in /linux/fs.h
 * */ 
struct file_operations peek_file_operation = {
    .open = peek_open,
    .read = peek_read,
    .write = peek_write,
    .release = peek_close,
};

static int __init peek_module_init(void) {
    printk(KERN_ALERT "Inside the peek_module_init function\n");
    //Registering the Character devie with the kernel
    // function parameters : Major Number, Name of the Driver, File operatons
    majorNumber = register_chrdev(0, DEVICE_NAME, &peek_file_operation);
    if(majorNumber < 0) {
        printk(KERN_ALERT "peek_module_init: failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "peek_module_init: registered correctly with major number %d\n", majorNumber);
    // Registering a class as shown by Karol
    peekClass = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(peekClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "peek_module_init: Failed to register device class\n");
        return PTR_ERR(peekClass);
    }
    printk(KERN_INFO "peek_module_init: device class registered correctly\n");
    //Registering the device driver
    peekDevice = device_create(peekClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if(IS_ERR(peekDevice)) {
        class_destroy(peekClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "peek_module_init: Failed to create the device\n");
        return PTR_ERR(peekDevice);
    }
    printk(KERN_INFO "peek_module_init: device class created correctly\n");
    mutex_init(&peekMutex); 
    return 0;
}
static void __exit peek_module_exit(void) {
    printk(KERN_ALERT "Inside the peek_module_exit function\n");
    //Unregistering the Character devie with the kernel
    // Reversing init
    mutex_destroy(&peekMutex);
    device_destroy(peekClass, MKDEV(majorNumber, 0));
    class_unregister(peekClass);
    class_destroy(peekClass); 
    unregister_chrdev(majorNumber, DEVICE_NAME); 
    printk(KERN_INFO "peek_module_exit: Goodbye!\n");
}
module_init(peek_module_init);
module_exit(peek_module_exit);