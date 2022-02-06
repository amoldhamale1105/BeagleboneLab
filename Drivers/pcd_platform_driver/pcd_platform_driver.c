#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

enum pcdev_names
{
    PCDEVA1x,
    PCDEVB1x,
    PCDEVC1x,
    PCDEVD1x
};

struct device_config
{
    int configItem1;
    int configItem2;
};

struct device_config pcdev_config[] = {
    {
        .configItem1 = 60,
        .configItem2 = 23
    },
    {
        .configItem1 = 50,
        .configItem2 = 13
    },
    {
        .configItem1 = 40,
        .configItem2 = 33
    },
    {
        .configItem1 = 30,
        .configItem2 = 43
    }
};

//Device private data structure
struct pcdev_private_data
{
    struct pcdev_platform_data pdata;
    char* buffer;
    dev_t dev_num;
    struct cdev cdev;
    struct mutex pcdev_lock;
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
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->pdata.size;
	pr_info("lseek requested\n");
	pr_info("current file position = %lld\n",filp->f_pos);
	if(offset > max_size || offset < 0)
		return -EINVAL;
	
	switch(whence)
	{
		case SEEK_SET:
		filp->f_pos = offset;
		break;
		
		case SEEK_CUR:
		if(filp->f_pos+offset > max_size || filp->f_pos+offset < 0)
			return -EINVAL; 
		filp->f_pos += offset;
		break;

		case SEEK_END:
		if(max_size+offset > max_size || max_size+offset < 0)
			return -EINVAL; 
		filp->f_pos = max_size + offset;
		break;
		
		default:
		return -EINVAL;
	}

	pr_info("new value of file position = %lld\n",filp->f_pos);
	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->pdata.size;
    
    mutex_lock(&pcdev_data->pcdev_lock);
	
    pr_info("read requested for %zu bytes\n", count);
	pr_info("Current file position = %lld\n", *f_pos);
	
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;
	
	if(copy_to_user(buff, pcdev_data->buffer+(*f_pos), count))
		return -EFAULT; 

	*f_pos += count;
	pr_info("Number of bytes successfully read = %zu\n", count);
	pr_info("Updated file position = %lld\n", *f_pos);

    mutex_unlock(&pcdev_data->pcdev_lock);

	//Return the number of bytes successfully read
	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->pdata.size;

    mutex_lock(&pcdev_data->pcdev_lock);
	pr_info("write requested for %zu bytes\n", count);
	pr_info("Current file position = %lld\n", *f_pos);
	
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;
	
	if(!count)
		return -ENOMEM;
	
	if(copy_from_user(&pcdev_data->buffer[*f_pos], buff, count))
		return -EFAULT; 

	*f_pos += count;
	pr_info("Number of bytes successfully written = %zu\n", count);
	pr_info("Updated file position = %lld\n", *f_pos);

    mutex_unlock(&pcdev_data->pcdev_lock);

	//Return the number of bytes successfully written
	return count;
}

static int check_permission(int dev_perm, int access_mode)
{
	if(dev_perm == RDWR)
		return 0;

	else if(dev_perm == RDONLY && (access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE))
		return 0;
	
	else if(dev_perm == WRONLY && !(access_mode & FMODE_READ) && (access_mode & FMODE_WRITE))
		return 0;

	return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
    int ret, minor_n;
	struct pcdev_private_data *pcdev_data;

	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access = %d\n",minor_n);
	
	//Extract dev private data ptr from its member cdev
	pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
	
	//Save ptr of dev private data for other file operation methods
	filp->private_data = pcdev_data;

	ret = check_permission(pcdev_data->pdata.perm, filp->f_mode);

	!ret ? pr_info("Open successful\n") : pr_info("Open unsuccessful\n");

	return ret;
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

    dev_data = (struct pcdev_private_data*)devm_kzalloc(&pdev->dev, sizeof(struct pcdev_private_data), GFP_KERNEL);
    if(!dev_data){
        pr_info("Can't allocate memory\n");
        ret = -ENOMEM;
        goto out;
    }

    mutex_init(&dev_data->pcdev_lock);

    //Save dev private data in the platform device driver data field
    //pdev->dev.driver_data = dev_data;
    dev_set_drvdata(&pdev->dev, dev_data);

    dev_data->pdata.size = pdata->size;
    dev_data->pdata.perm = pdata->perm;
    dev_data->pdata.serial_number = pdata->serial_number;

    pr_info("Device serial number = %s\n",dev_data->pdata.serial_number);
    pr_info("Device size = %d\n",dev_data->pdata.size);
    pr_info("Device permission = %d\n",dev_data->pdata.perm);

    pr_info("ConfigItem1 = %d\n", pcdev_config[pdev->id_entry->driver_data].configItem1);
    pr_info("ConfigItem2 = %d\n", pcdev_config[pdev->id_entry->driver_data].configItem2);
    
    //Dynamically allocate mem for device buffer using size info and platform data
    dev_data->buffer = (char*)devm_kzalloc(&pdev->dev, dev_data->pdata.size, GFP_KERNEL);
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
		goto out;
        
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

    pcdrv_data.total_devices--;

    pr_info("Device removed\n");
    return 0;
}

struct platform_device_id pcdev_ids[] = {
    {
        .name = "pcdev-A1x",
        .driver_data = PCDEVA1x
    },
    {
        .name = "pcdev-B1x",
        .driver_data = PCDEVB1x
    },
    {
        .name = "pcdev-C1x",
        .driver_data = PCDEVC1x
    },
    {
        .name = "pcdev-D1x",
        .driver_data = PCDEVD1x
    },
    { } //null terminate the array
};

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .id_table = pcdev_ids,
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
