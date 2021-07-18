#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt, __func__

#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 256
#define MEM_SIZE_MAX_PCDEV4 128
#define NO_OF_DEVICES 4

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];

struct pcdev_private_data {
    char *buffer;
    unsigned size;
    int perm;
    const char *serial_number;
    struct cdev cdev;
};

struct pcdrv_private_data {
    int total_devices;
    /* This holds device number */
    dev_t dev_number;
    struct class *pcd_class;
    struct device *pcd_device;
    struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

struct pcdrv_private_data drv_pvt_data = {
    .total_devices = NO_OF_DEVICES,
    .pcdev_data = {
        [0] = {
            .buffer = device_buffer_pcdev1,
            .size = MEM_SIZE_MAX_PCDEV1,
            .serial_number = "PCDEV1XYZ1",
            .perm = RDONLY
        },
        [1] = {
            .buffer = device_buffer_pcdev2,
            .size = MEM_SIZE_MAX_PCDEV2,
            .serial_number = "PCDEV1XYZ2",
            .perm = WRONLY
        },
        [2] = {
            .buffer = device_buffer_pcdev3,
            .size = MEM_SIZE_MAX_PCDEV3,
            .serial_number = "PCDEV1XYZ3",
            .perm = RDWR
        },
        [3] = {
            .buffer = device_buffer_pcdev4,
            .size = MEM_SIZE_MAX_PCDEV4,
            .serial_number = "PCDEV1XYZ4",
            .perm = RDWR
        }
    }
};

loff_t pcd_lseek(struct file *filp, loff_t off, int whence)
{
    struct pcdev_private_data *pcdev_data;
    int max_size;
    loff_t temp;
    pr_info("Current file position: %lld\n", filp->f_pos);

    pcdev_data = (struct pcdev_private_data *)filp->private_data;
    max_size = pcdev_data->size;

    switch (whence)
    {
    case SEEK_SET:
        if ((off > max_size) || (off < 0))
        {
            return -EINVAL;
        }
        filp->f_pos = off;
        break;
    case SEEK_CUR:
        temp = filp->f_pos + off;
        if((temp > max_size) || (temp < 0)) {
            return -EINVAL;
        }
        filp->f_pos = temp;
        break;
    case SEEK_END:
        temp = max_size + off;
        if((temp > max_size) || (temp < 0)) {
            return -EINVAL;
        }
        filp->f_pos = temp;
        break;
    default:
        return -EINVAL;
    }

    pr_info("New file position: %lld\n", filp->f_pos);
    return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    struct pcdev_private_data *pcdev_data;
    int max_size;
    pr_info("Read requested for %zu bytes\n", count);
    pr_info("Current file position = %lld\n", *f_pos);

    pcdev_data = (struct pcdev_private_data *)filp->private_data;
    max_size = pcdev_data->size;

    /* Adjust the count */
    if ((*f_pos + count) > max_size)
    {
        count = max_size - *f_pos;
    }

    /* copy to user */
    if (copy_to_user(buff, &pcdev_data->buffer[*f_pos], count))
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
    struct pcdev_private_data *pcdev_data;
    int max_size;
    pr_info("Write requested for %zu bytes\n", count);
    pr_info("Current file position = %lld\n", *f_pos);

    pcdev_data = (struct pcdev_private_data *)filp->private_data;
    max_size = pcdev_data->size;


    /* Adjust the count */
    if ((*f_pos + count) > max_size)
    {
        count = max_size - *f_pos;
    }

    if (!count)
    {
        return -ENOMEM;
    }

    /* copy to user */
    if (copy_from_user(&pcdev_data->buffer[*f_pos], buff, count))
    {
        return -EFAULT;
    }

    /*Update the curent offset*/
    *f_pos += count;

    pr_info("No of bytes wrote = %zu\n", count);
    pr_info("New File position = %lld\n", *f_pos);
    return count;
}

int check_permission(int dev_perm, int acc_mode)
{
    if(dev_perm == RDWR)
        return 0;
    if((dev_perm == RDONLY) && ((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
        return 0;
    if((dev_perm == WRONLY) && (!(acc_mode & FMODE_READ) && (acc_mode & FMODE_WRITE)))
        return 0;
    return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    int minor_n;
    struct pcdev_private_data *pcdev_data;

    minor_n = MINOR(inode->i_rdev);
    pr_info("Minor Number: %d\n", minor_n);
    pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);

    /* To supply device data to other calls */
    filp->private_data = pcdev_data;

    /* Check Permissions */
    ret = check_permission(pcdev_data->perm, filp->f_mode);

    return ret;
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
    .owner = THIS_MODULE
};

static int __init pcd_init(void)
{
    int ret, i;
    /* Dynamically Allocate a device number */
    ret = alloc_chrdev_region(&drv_pvt_data.dev_number, 0, NO_OF_DEVICES, "pcd_devices");
    if (ret < 0)
    {
        pr_err("alloc_chrdev_region Failed\n");
        goto out;
    }

    /* Create Device class under /sys/class/  */
    drv_pvt_data.pcd_class = class_create(THIS_MODULE, "pcd_class");
    if (IS_ERR(drv_pvt_data.pcd_class))
    {
            pr_err("Class creation Failed\n");
            ret = PTR_ERR(drv_pvt_data.pcd_class);
            goto unreg_chrdev;
    }

    for (i = 0; i < NO_OF_DEVICES; i++)
    {
        pr_info("[PCD Device Number]: <Major:Minor> = %d:%d\n", MAJOR(drv_pvt_data.dev_number + i), MINOR(drv_pvt_data.dev_number + i));

        /* initialize a device structure */
        cdev_init(&drv_pvt_data.pcdev_data[i].cdev, &pcd_fops);

        /* Register Cdev Structure with VFS */
        drv_pvt_data.pcdev_data[i].cdev.owner = THIS_MODULE;

        ret = cdev_add(&drv_pvt_data.pcdev_data[i].cdev, drv_pvt_data.dev_number + i, 1);
        if (ret < 0)
        {
            pr_err("cdev_add Failed\n");
            goto cdev_del;
        }


        /* Populate the sysfs with device information */
        drv_pvt_data.pcd_device = device_create(drv_pvt_data.pcd_class, NULL, drv_pvt_data.dev_number + i, NULL, "pcdev-%d", i);
        if (IS_ERR(drv_pvt_data.pcd_device))
        {
            pr_err("Device creation Failed\n");
            ret = PTR_ERR(drv_pvt_data.pcd_device);
            goto del_class;
        }
    }
    return 0;

cdev_del:
del_class:
    for(;i>=0;i--){
        device_destroy(drv_pvt_data.pcd_class, drv_pvt_data.dev_number+i);
        cdev_del(&drv_pvt_data.pcdev_data[i].cdev);
    }
    class_destroy(drv_pvt_data.pcd_class);
unreg_chrdev:
    unregister_chrdev_region(drv_pvt_data.dev_number, NO_OF_DEVICES);
out:
    pr_err("Module Insertion Failed\n");
    return ret;
}

static void __exit pcd_exit(void)
{
    int i;
    for(i=0;i<NO_OF_DEVICES;i++){
        device_destroy(drv_pvt_data.pcd_class, drv_pvt_data.dev_number+i);
        cdev_del(&drv_pvt_data.pcdev_data[i].cdev);
    }
    class_destroy(drv_pvt_data.pcd_class);
    unregister_chrdev_region(drv_pvt_data.dev_number, NO_OF_DEVICES);
    pr_info("PCD: Leaving Kernel World\n");
}

module_init(pcd_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mukesh Kumar");
MODULE_DESCRIPTION("A Pseudo Character device to handle N devices");
MODULE_INFO(board, "Beaglebone");
