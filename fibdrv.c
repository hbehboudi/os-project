#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Amir Afsari, Hamed Alimohammadzadeh, Hossein Behboudi");

#define DEV_FIBONACCI_NAME "fibonacci"

static dev_t fibonacci_dev = 0;
static struct cdev *fibonacci_cdev;
static struct class *fibonacci_class;
static DEFINE_MUTEX(fibonacci_mutex);

static long long fibonacci_sequence(long long number) {
    long long previousNum = 0;
    long long currentNum = 1;

    if (number == 0) {
        return previousNum;
    }

    for (int i = 0; i < number - 1; i++) {
        currentNum += previousNum;
        previousNum = currentNum - previousNum;
    }

    return currentNum;
}

static ssize_t fibonacci_read(struct file *file, char *buf, size_t size, loff_t *offset) {
    return (ssize_t) fibonacci_sequence(*offset);
}

static ssize_t fibonacci_write(struct file *file, const char *buf, size_t size, loff_t *offset) {
    return 1;
}

static int fibonacci_open(struct inode *inode, struct file *file) {
    if (!mutex_trylock(&fibonacci_mutex)) {
        printk(KERN_ALERT "fibonacci driver is in use");
        return -EBUSY;
    }

    return 0;
}

static int fibonacci_release(struct inode *inode, struct file *file) {
    mutex_unlock(&fibonacci_mutex);
    return 0;
}

static loff_t fibonacci_device_lseek(struct file *file, loff_t offset, int orig) {
    loff_t new_pos = 0;

    switch (orig) {
        case 0:
            new_pos = offset;
            break;
        case 1:
            new_pos = file->f_pos + offset;
            break;
        case 2:
            new_pos = 92 - offset;
            break;
    }

    file->f_pos = new_pos;
    return new_pos;
}

const struct file_operations fibonacci_fops = {
        .owner = THIS_MODULE,
        .read = fibonacci_read,
        .write = fibonacci_write,
        .open = fibonacci_open,
        .release = fibonacci_release,
        .llseek = fibonacci_device_lseek,
};

static int __init fibonacci_init(void) {
    mutex_init(&fibonacci_mutex);

    int result = alloc_chrdev_region(&fibonacci_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (result < 0) {
        printk(KERN_ALERT "Failed to register the fibonacci char device. result = %i", result);
        return result;
    }

    fibonacci_cdev = cdev_alloc();

    if (fibonacci_cdev == NULL) {
        result = -1;
        printk(KERN_ALERT "Failed to alloc cdev");
        unregister_chrdev_region(fibonacci_dev, 1);
        return result;
    }

    cdev_init(fibonacci_cdev, &fibonacci_fops);

    result = cdev_add(fibonacci_cdev, fibonacci_dev, 1);

    if (result < 0) {
        result = -2;
        printk(KERN_ALERT "Failed to add cdev");
        unregister_chrdev_region(fibonacci_dev, 1);
        return result;
    }

    fibonacci_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fibonacci_class) {
        result = -3;
        printk(KERN_ALERT "Failed to create device class");
        cdev_del(fibonacci_cdev);
        unregister_chrdev_region(fibonacci_dev, 1);
        return result;
    }

    if (!device_create(fibonacci_class, NULL, fibonacci_dev, NULL, DEV_FIBONACCI_NAME)) {
        result = -4;
        printk(KERN_ALERT "Failed to create device");
        class_destroy(fibonacci_class);
        cdev_del(fibonacci_cdev);
        unregister_chrdev_region(fibonacci_dev, 1);
        return result;
    }

    return result;
}

static void __exit fibonacci_exit(void) {
    mutex_destroy(&fibonacci_mutex);
    device_destroy(fibonacci_class, fibonacci_dev);
    class_destroy(fibonacci_class);
    cdev_del(fibonacci_cdev);
    unregister_chrdev_region(fibonacci_dev, 1);
}

module_init(fibonacci_init);
module_exit(fibonacci_exit);

