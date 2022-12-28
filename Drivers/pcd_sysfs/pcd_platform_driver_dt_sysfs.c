#include "pcd_platform_driver_dt_sysfs.h"
#include "pcd_syscalls.h"

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

struct pcdrv_private_data pcdrv_data;

struct file_operations pcd_fops=
{
	.open = pcd_open,
	.write = pcd_write,
	.read = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};

ssize_t show_max_size(struct device *dev, struct device_attribute *attr, char* buf)
{
    ssize_t ret;
    struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
    mutex_lock(&dev_data->pcd_lock);
    ret = sprintf(buf,"%d\n",dev_data->pdata.size);
    mutex_unlock(&dev_data->pcd_lock);
    return ret;
}

ssize_t show_serial_num(struct device *dev, struct device_attribute *attr, char* buf)
{
    ssize_t ret;
    struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
    mutex_lock(&dev_data->pcd_lock);
    ret = sprintf(buf,"%s\n",dev_data->pdata.serial_number);
    mutex_unlock(&dev_data->pcd_lock);
    return ret;
}

ssize_t store_max_size(struct device *dev, struct device_attribute* attr, const char* buf, size_t count)
{
    long result;
    int ret;
    struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);

    mutex_lock(&dev_data->pcd_lock);
    //kernel method to convert string to long
    ret = kstrtol(buf, 0, &result);
    if(ret)
        return ret;
    
    dev_data->pdata.size = result;
    //Reallocate memory for buffer since there's a change of size
    dev_data->buffer = krealloc(dev_data->buffer, dev_data->pdata.size, GFP_KERNEL);

    mutex_unlock(&dev_data->pcd_lock);
    return count;
}

//Create 2 vars of struct device attribute
static DEVICE_ATTR(max_size, S_IRUGO | S_IWUSR, show_max_size, store_max_size);
static DEVICE_ATTR(serial_num, S_IRUGO, show_serial_num, NULL);

struct attribute* pcd_attrs[] = {
    &dev_attr_max_size.attr,
    &dev_attr_serial_num.attr,
    NULL
};

struct attribute_group pcd_attr_group = {
    .attrs = pcd_attrs
};

int pcd_sysfs_create_files(struct device* pcd_dev)
{
#if 0
    int ret;
    ret = sysfs_create_file(&pcd_dev->kobj, &dev_attr_max_size.attr);
    if(ret)
        return ret;

    return sysfs_create_file(&pcd_dev->kobj, &dev_attr_serial_num.attr);
#endif
    return sysfs_create_group(&pcd_dev->kobj, &pcd_attr_group);
}

static struct pcdev_platform_data* pcdev_get_pltdata_from_dt(struct device *dev)
{
    struct device_node *dev_node = dev->of_node;
    struct pcdev_platform_data *pdata;
    
    //When probe was called because of device setup than a tree
    if(!dev_node)
        return NULL;

    pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
    if(!pdata){
        dev_info(dev, "Can't allocate memory\n");
        return ERR_PTR(-ENOMEM);
    }
    
    if(of_property_read_string(dev_node, "org,device-serial-num", &pdata->serial_number)){
        dev_info(dev, "Missing serial number property");
        return ERR_PTR(-EINVAL);
    }
    
    if(of_property_read_u32(dev_node, "org,size", &pdata->size)){
        dev_info(dev, "Missing size property");
        return ERR_PTR(-EINVAL);
    }
    
    if(of_property_read_u32(dev_node, "org,perm", &pdata->perm)){
        dev_info(dev, "Missing permission property");
        return ERR_PTR(-EINVAL);
    }

    return pdata;
}

//Called when matching device is found
int pcd_platform_driver_probe(struct platform_device* pdev)
{
    int ret;
    struct pcdev_private_data* dev_data;
    struct pcdev_platform_data *pdata;
    struct device *dev = &pdev->dev;
    struct of_device_id *match;
    int driver_data;
    
    dev_info(dev, "Device detected\n");

    //Match will be NULL if kernel does not support device tree i.e CONFIG_OF is off
    match = (struct of_device_id*)of_match_device(pdev->dev.driver->of_match_table, dev);

    if(match){
        //Get platform data from device tree
        pdata = pcdev_get_pltdata_from_dt(dev);
        if(IS_ERR(pdata))
            return PTR_ERR(pdata);
        driver_data = (int)match->data;
    }
    else{
        //Get the platform data from device setup
        pdata = (struct pcdev_platform_data*)dev_get_platdata(dev);
        driver_data = pdev->id_entry->driver_data;
    }

    if(!pdata){
        dev_info(dev, "No platform data available\n");
        ret = -EINVAL;
        goto out;
    }


    dev_data = (struct pcdev_private_data*)devm_kzalloc(&pdev->dev, sizeof(struct pcdev_private_data), GFP_KERNEL);
    if(!dev_data){
        dev_info(dev, "Can't allocate memory\n");
        ret = -ENOMEM;
        goto out;
    }

    mutex_init(&dev_data->pcd_lock);
    
    //Save dev private data in the platform device driver data field
    //pdev->dev.driver_data = dev_data;
    dev_set_drvdata(&pdev->dev, dev_data);

    dev_data->pdata.size = pdata->size;
    dev_data->pdata.perm = pdata->perm;
    dev_data->pdata.serial_number = pdata->serial_number;

    pr_info("Device serial number = %s\n",dev_data->pdata.serial_number);
    pr_info("Device size = %d\n",dev_data->pdata.size);
    pr_info("Device permission = %d\n",dev_data->pdata.perm);

    pr_info("ConfigItem1 = %d\n", pcdev_config[driver_data].configItem1);
    pr_info("ConfigItem2 = %d\n", pcdev_config[driver_data].configItem2);
    
    //Dynamically allocate mem for device buffer using size info and platform data
    dev_data->buffer = (char*)devm_kzalloc(&pdev->dev, dev_data->pdata.size, GFP_KERNEL);
    if(!dev_data->buffer){
        pr_info("Can't allocate memory\n");
        ret = -ENOMEM;
        goto out;
    }

    //Get device number
    dev_data->dev_num = pcdrv_data.device_num_base + pcdrv_data.total_devices;

    //cdev init and add
    cdev_init(&dev_data->cdev, &pcd_fops);
    dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
	if(ret < 0)
		goto out;
        
    //Create device file for the detected platform device
    pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, dev, dev_data->dev_num, NULL, "pcdev-%d",pcdrv_data.total_devices);
    if(IS_ERR(pcdrv_data.device_pcd))
    {
        dev_err(dev, "Device creation failed!");
        ret = PTR_ERR(pcdrv_data.device_pcd);
        goto cdev_del;
    }

    pcdrv_data.total_devices++;

    ret = pcd_sysfs_create_files(pcdrv_data.device_pcd);
    if (ret){
        device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);
        goto out;
    }

    pr_info("Probe successful!\n");
    return 0;

cdev_del:
    cdev_del(&dev_data->cdev);
out:
    dev_info(dev, "Device probe failed\n");
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
    
    dev_info(&pdev->dev, "Device removed\n");
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

struct of_device_id org_pcdev_dt_match[] = {
    {
        .compatible = "pcdev-A1x",
        .data = (void*)PCDEVA1x
    },
    {
        .compatible = "pcdev-B1x",
        .data = (void*)PCDEVB1x
    },
    {
        .compatible = "pcdev-C1x",
        .data = (void*)PCDEVC1x
    },
    {
        .compatible = "pcdev-D1x",
        .data = (void*)PCDEVD1x
    },
    {}
};

struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .id_table = pcdev_ids,
    .driver = {
        .name = "pseudo-char-device",
        .of_match_table = of_match_ptr(org_pcdev_dt_match)
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
MODULE_AUTHOR("Amol Dhamale");
MODULE_DESCRIPTION("Platform driver for pseudo char devices with attributes");
MODULE_INFO(board, "BBB Rev A5");
