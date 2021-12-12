#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

//Device private data structure
struct pcdev_private_data
{
    struct pcdev_platform_data pdata;
    char* buffer;
    dev_t dev_num;
    struct cdev cdev;
};

//Driver private data structure
struct pcdrv_private_data
{
    int total_devices;
    dev_t device_num_base;
    struct class *class_pcd;
    struct device *device_pcd;
};

struct pcdrv_private_data pcdrv_data;

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
    return 0;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    return 0;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    return -ENOMEM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("release successful\n");
	return 0;
}

struct file_operations pcd_fops=
{
	.open = pcd_open,
	.write = pcd_write,
	.read = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};

//Called when matching device is found
int pcd_platform_driver_probe(struct platform_device* pdev)
{
    int ret;
    struct pcdev_private_data* dev_data;
    struct pcdev_platform_data *pdata;
    
    pr_info("Device detected\n");

    //Get the platform data
    pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);
    if(!pdata){
        pr_info("No platform data available\n");
        ret = -EINVAL;
        goto out;
    }

    dev_data = (struct pcdev_private_data*)kzalloc(sizeof(struct pcdev_private_data), GFP_KERNEL);
    if(!dev_data){
        pr_info("Can't allocate memory\n");
        ret = -ENOMEM;
        goto dev_data_free;
    }

    //Save dev private data in the platform device driver data field
    //pdev->dev.driver_data = dev_data;
    dev_set_drvdata(&pdev->dev, dev_data);

    dev_data->pdata.size = pdata->size;
    dev_data->pdata.perm = pdata->perm;
    dev_data->pdata.serial_number = pdata->serial_number;

    pr_info("Device serial number = %s\n",dev_data->pdata.serial_number);
    pr_info("Device size = %d\n",dev_data->pdata.size);
    pr_info("Device permission = %d\n",dev_data->pdata.perm);
    
    //Dynamically allocate mem for device buffer using size info and platform data
    dev_data->buffer = (char*)kzalloc(sizeof(dev_data->pdata.size), GFP_KERNEL);
    if(!dev_data->buffer){
        pr_info("Can't allocate memory\n");
        ret = -ENOMEM;
        goto out;
    }

    //Get device number
    dev_data->dev_num = pcdrv_data.device_num_base + pdev->id;

    //cdev init and add
    cdev_init(&dev_data->cdev, &pcd_fops);
    dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
	if(ret < 0)
		goto buffer_free;
        
    //Create device file for the detected platform device
    pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL, "pcdev-%d",pdev->id);
    if(IS_ERR(pcdrv_data.device_pcd))
    {
        pr_err("Device creation failed!");
        ret = PTR_ERR(pcdrv_data.device_pcd);
        goto cdev_del;
    }

    pcdrv_data.total_devices++;

    pr_info("Probe successful!\n");
    return 0;

cdev_del:
    cdev_del(&dev_data->cdev);
buffer_free:
    kfree(dev_data->buffer);
dev_data_free:
    kfree(dev_data);
out:
    pr_info("Device probe failed\n");
    return ret;
}

//Remove gets called when device is removed from system
int pcd_platform_driver_remove(struct platform_device* pdev)
{
    struct pcdev_private_data *dev_data = (struct pcdev_private_data*)pdev->dev.driver_data;
    //Remove a device created with device_create()
    device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);

    //Remove cdev entry from system
    cdev_del(&dev_data->cdev);

    //Free memory held by device
    kfree(dev_data->buffer);
    kfree(dev_data);

    pcdrv_data.total_devices--;

    pr_info("Device removed\n");
    return 0;
}

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .driver = {
        .name = "pseudo-char-device"
    }
};


static int __init pcd_platform_driver_init(void)
{
    int ret;
    //Dynamically allocate a device number for MAX_DEVICES
    ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, MAX_DEVICES, "pcd_devices");
    if(ret < 0)
		goto out;
    
    //Create device class directory under /sys/class
    pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("Class creation failed!");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		goto unreg_chrdev;
	}

    //Register platform driver
    platform_driver_register(&pcd_platform_driver);

    pr_info("pcd platform driver loaded\n");
    return 0;

unreg_chrdev:
	unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
out:
	pr_info("Module insertion failed!\n");
	return ret;
}

static void __exit pcd_platform_driver_cleanup(void)
{
    platform_driver_unregister(&pcd_platform_driver);
    class_destroy(pcdrv_data.class_pcd);
	unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
    pr_info("pcd platform driver unloaded\n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amol");
MODULE_DESCRIPTION("Platform driver for platform devices");
MODULE_INFO(board, "BBB Rev A5");
