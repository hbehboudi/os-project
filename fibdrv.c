#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fdtable.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Amir Afsari, Hamed Alimohammadzadeh, Hossein Behboudi");

#define DEV_NAME "process"
#define BUF_SIZE 4096

static dev_t process_dev = 0;
static struct cdev *process_cdev;
static struct class *process_class;
static DEFINE_MUTEX(process_mutex);

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

static void add_state(char *str1, char* result) {
    int i, j, index = 0, size = 5;
    char *str2 = "State";

    for (i = 0; i < BUF_SIZE - size; i++) {
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

        result[index++] = '\n';

        do
        {
            result[index] = str1[i];
            index++;
            i++;
        } while (str1[i] != '\n' && str1[i] != '\0');

        break;
    }
    
    result[index] = '\n';
    result[index + 1] = '\0';
}

static ssize_t process_read(struct file *file_ptr, char __user *user_buffer, size_t count, loff_t *possition) {
    char temp[BUF_SIZE] = "";
    char result[BUF_SIZE] = "";

    long long pid = *possition;

    struct pid *pid_struct = find_get_pid(pid);

    if (pid_struct == NULL) {
        sprintf(result, "Pid does not exist.");
        
        if (copy_to_user(user_buffer, result, count) != 0) {
            return -EFAULT;
        }

        return -1;
    }

    struct task_struct *task = pid_task(pid_struct, PIDTYPE_PID);

    // State
    char status_path[32];
    sprintf(status_path, "/proc/%lld/status", pid);
    struct file *status_file = file_open(status_path, O_RDONLY, 0);
    file_read(status_file, 0, temp, BUF_SIZE);
    file_close(status_file);

    add_state(temp, result);

    if (task->state == -1) {
        strcat((char*)result, "unrunnable\n");
    } else if (task->state == 0) {
        strcat((char*)result, "runnable\n");
    } else {
        strcat((char*)result, "stopped\n");
    }

    // start_time
    sprintf(temp, "\nstart_time: %lld\n", task->start_time);
    strcat((char*)result, temp);

    // rt_priority
    sprintf(temp, "\nrt_priority: %d\n", task->rt_priority);
    strcat((char*)result, temp);

    // open file information
    struct fdtable* filestable = files_fdtable(task->files);

    strcat((char*)result, "\nopen files:");

    int i = 0;
    while (filestable->fd[i] != NULL){
        struct path files_path = filestable->fd[i]->f_path;
        char *cwd = d_path(&files_path, temp, 128 * sizeof(char));
        strcat((char*)result, "\n");
        strcat((char*)result, cwd);
        i++;
    }

    if (copy_to_user(user_buffer, result, count) != 0) {
        return -EFAULT;
    }

   return count;
}

static loff_t process_device_lseek(struct file *file, loff_t offset, int orig) {
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

const struct file_operations process_fops = {
    .owner = THIS_MODULE,
    .read = process_read,
    .llseek = process_device_lseek,
};

static int __init process_init(void) {
    mutex_init(&process_mutex);

    int result = alloc_chrdev_region(&process_dev, 0, 1, DEV_NAME);

    if (result < 0) {
        printk(KERN_ALERT "Failed to register the process char device. result = %i", result);
        return result;
    }

    process_cdev = cdev_alloc();

    if (process_cdev == NULL) {
        result = -1;
        printk(KERN_ALERT "Failed to alloc cdev");
        unregister_chrdev_region(process_dev, 1);
        return result;
    }

    cdev_init(process_cdev, &process_fops);

    result = cdev_add(process_cdev, process_dev, 1);

    if (result < 0) {
        result = -2;
        printk(KERN_ALERT "Failed to add cdev");
        unregister_chrdev_region(process_dev, 1);
        return result;
    }

    process_class = class_create(THIS_MODULE, DEV_NAME);

    if (!process_class) {
        result = -3;
        printk(KERN_ALERT "Failed to create device class");
        cdev_del(process_cdev);
        unregister_chrdev_region(process_dev, 1);
        return result;
    }

    if (!device_create(process_class, NULL, process_dev, NULL, DEV_NAME)) {
        result = -4;
        printk(KERN_ALERT "Failed to create device");
        class_destroy(process_class);
        cdev_del(process_cdev);
        unregister_chrdev_region(process_dev, 1);
        return result;
    }

    return result;
}

static void __exit process_exit(void) {
    mutex_destroy(&process_mutex);
    device_destroy(process_class, process_dev);
    class_destroy(process_class);
    cdev_del(process_cdev);
    unregister_chrdev_region(process_dev, 1);
}

module_init(process_init);
module_exit(process_exit);
