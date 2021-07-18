#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt, __func__

#define DEV_MEM_SIZE 512

char pseudo_device[DEV_MEM_SIZE];

/* This holds device number */
dev_t dev_number;

/* Cdev Variable */
struct cdev pcd_cdev;
loff_t pcd_lseek(struct file *filp, loff_t off, int whence)
{
    loff_t temp;
    pr_info("Current file position:\n", filp->f_pos);
    switch (whence)
    {
    case SEEK_SET:
        if ((off > DEV_MEM_SIZE) || (off < 0))
        {
            return -EINVAL;
        }
        filp->f_pos = off;
        break;
    case SEEK_CUR:
        temp = filp->f_pos + off;
        if((temp > DEV_MEM_SIZE) || (temp < 0)) {
            return -EINVAL;
        }
        filp->f_pos = temp;
        break;
    case SEEK_END:
        temp = DEV_MEM_SIZE + off;
        if((temp > DEV_MEM_SIZE) || (temp < 0)) {
            return -EINVAL;
        }
        filp->f_pos = temp;
        break;
    default:
        return -EINVAL;
    }

    pr_info("New file position:\n", filp->f_pos);
    return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("Read requested for %zu bytes\n", count);
    pr_info("Current file position = %lld\n", *f_pos);
    /* Adjust the count */
    if ((*f_pos + count) > DEV_MEM_SIZE)
    {
        count = DEV_MEM_SIZE - *f_pos;
    }

    /* copy to user */
    if (copy_to_user(buff, &pseudo_device[*f_pos], count))
    {
        return -EFAULT;
    }

    /*Update the curent offset*/
    *f_pos += count;

    pr_info("No of bytes read = %zu\n", count);
    pr_info("New File position = %lld\n", *f_pos);
    return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("Write requested for %zu bytes\n", count);
    pr_info("Current file position = %lld\n", *f_pos);
    /* Adjust the count */
    if ((*f_pos + count) > DEV_MEM_SIZE)
    {
        count = DEV_MEM_SIZE - *f_pos;
    }

    if (!count)
    {
        return -ENOMEM;
    }

    /* copy to user */
    if (copy_from_user(&pseudo_device[*f_pos], buff, count))
    {
        return -EFAULT;
    }

    /*Update the curent offset*/
    *f_pos += count;

    pr_info("No of bytes wrote = %zu\n", count);
    pr_info("New File position = %lld\n", *f_pos);
    return count;
}

int pcd_open(struct inode *inode, struct file *filp)
{
    pr_info("Open Successfull\n");
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
    pr_info("Release Success\n");

    return 0;
}

/* File Operations of the driver*/
struct file_operations pcd_fops = {
    .llseek = pcd_lseek,
    .open = pcd_open,
    .release = pcd_release,
    .read = pcd_read,
    .write = pcd_write,
    .owner = THIS_MODULE};

struct class *pcd_class;

struct device *pcd_device;

static int __init pcd_init(void)
{
    int ret;
    /*1. Dynamically Allocate a device number */
    ret = alloc_chrdev_region(&dev_number, 0, 1, "pcd");
    if(ret < 0) {
        pr_err("alloc_chrdev_region Failed\n");
        goto out;
    }
    pr_info("[PCD Device Number]: <Major:Minor> = %d:%d\n", MAJOR(dev_number), MINOR(dev_number));

    /*2. initialize a device structure */
    cdev_init(&pcd_cdev, &pcd_fops);

    /*3. Register Cdev Structure with VFS */
    pcd_cdev.owner = THIS_MODULE;
    ret = cdev_add(&pcd_cdev, dev_number, 1);
    if(ret < 0) {
        pr_err("cdev_add Failed\n");
        goto unreg_chrdev;
    }
    /*4. Create Device class under /sys/class/  */
    pcd_class = class_create(THIS_MODULE, "pcd_class");
    if(IS_ERR(pcd_class)) {
        pr_err("Class creation Failed\n");
        ret = PTR_ERR(pcd_class);
        goto del_dev;
    }

    /*5. Populate the sysfs with device information */
    pcd_device = device_create(pcd_class, NULL, dev_number, NULL, "pcd");
        if(IS_ERR(pcd_device)) {
        pr_err("Device creation Failed\n");
        ret = PTR_ERR(pcd_device);
        goto del_class;
    }
    return 0;

del_class: 
    class_destroy(pcd_class);
del_dev:
    cdev_del(&pcd_cdev);
unreg_chrdev:
    unregister_chrdev_region(dev_number, 1);
out:
    pr_err("Module Insertion Failed\n");
    return ret;
}

static void __exit pcd_exit(void)
{
    device_destroy(pcd_class, dev_number);
    class_destroy(pcd_class);
    cdev_del(&pcd_cdev);
    unregister_chrdev_region(dev_number, 1);
    pr_info("PCD: Leaving Kernel World\n");
}

module_init(pcd_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mukesh Kumar");
MODULE_DESCRIPTION("A SImple Hello Woprld Kernel");
MODULE_INFO(board, "Beaglebone");
