#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/device.h>
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
    pr_info("Lseek Requested:\n");
    return 0;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t len, loff_t *offset)
{
    pr_info("Read Requested\n");
    return 0;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t len, loff_t *offset)
{
    pr_info("Write requested\n");
    return 0;
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
    /*1. Dynamically Allocate a device number */
    alloc_chrdev_region(&dev_number, 0, 1, "pcd");
    pr_info("[PCD Device Number]: <Major:Minor> = %d:%d\n", MAJOR(dev_number), MINOR(dev_number));

    /*2. initialize a device structure */
    cdev_init(&pcd_cdev, &pcd_fops);

    /*3. Register Cdev Structure with VFS */
    pcd_cdev.owner = THIS_MODULE;
    cdev_add(&pcd_cdev, dev_number, 1);

    /*4. Create Device class under /sys/class/  */
    pcd_class = class_create(THIS_MODULE, "pcd_class");

    /*5. Populate the sysfs with device information */
    pcd_device = device_create(pcd_class, NULL, dev_number, NULL, "pcd");
    return 0;
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
