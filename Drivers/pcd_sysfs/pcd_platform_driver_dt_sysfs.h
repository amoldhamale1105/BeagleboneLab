#ifndef PCD_PLATFORM_DT_SYSFS_H
#define PCD_PLATFORM_DT_SYSFS_H

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
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

//Device private data structure
struct pcdev_private_data
{
    struct pcdev_platform_data pdata;
    char* buffer;
    dev_t dev_num;
    struct cdev cdev;
    struct mutex pcd_lock;
};

//Driver private data structure
struct pcdrv_private_data
{
    int total_devices;
    dev_t device_num_base;
    struct class *class_pcd;
    struct device *device_pcd;
};

#endif
