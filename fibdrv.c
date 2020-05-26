#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <asm/segment.h>
#include <asm/uaccess.h> 
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/dirent.h>
#include <linux/stat.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Amir Afsari, Hamed Alimohammadzadeh, Hossein Behboudi");

#define DEV_FIBONACCI_NAME "fibonacci"
#define bufSize 1024

static dev_t fibonacci_dev = 0;
static struct cdev *fibonacci_cdev;
static struct class *fibonacci_class;
static DEFINE_MUTEX(fibonacci_mutex);

struct file *file_open(const char *path, int flags, int rights) 
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);

    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    
    return filp;
}

void file_close(struct file *file) 
{
    filp_close(file, NULL);
}

int file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size) 
{
    mm_segment_t oldfs;
    
    oldfs = get_fs();
    set_fs(get_ds());

    int ret = vfs_read(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

static int add_to_result(char *str1, char *str2, char* result, int size, int index) {
    int i, j;

    for (i = 0; i < bufSize - size; i++) {
        int find = 1;

        for (j = 0; j < size; j++) {
            if (str1[i + j] != str2[j]) {
                find = 0;
                break;
            }
        }

        if (find == 0) {
            continue;
        }

        do
        {
            result[index] = str1[i];
            index++;
            i++;
        } while (str1[i] != '\n' && str1[i] != '\0');

        break;
    }

    return index;
}

static ssize_t fibonacci_read(struct file *file_ptr, char __user *user_buffer, size_t count, loff_t *possition) {
    char str[bufSize] = "";
    char result[bufSize] = "";

    char tmp[32];
    sprintf(tmp, "/proc/%lld/status", *possition);

//  ********
    struct file *status_file = file_open(tmp, O_RDONLY, 0);
    file_read(status_file, 0, str, bufSize);
    file_close(status_file);

    int index = add_to_result(str, "State", result, 5, 0);
    
    result[index + 1] = "\0";
//  ********

    if (copy_to_user(user_buffer, result, count) != 0) {
        return -EFAULT;
    }

   return count;
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

