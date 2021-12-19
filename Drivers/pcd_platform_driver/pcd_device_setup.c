#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

void pcdev_release(struct device *dev)
{
    pr_info("Device released\n");
}

//Create 2 platform data
struct pcdev_platform_data pcdev_pdata[] = {
    {
        .size = 512,
        .perm = RDWR,
        .serial_number = "PCDEVABC1111"
    },
    {
        .size = 1024,
        .perm = RDWR,
        .serial_number = "PCDEVXYZ2222"
    },
    {
        .size = 128,
        .perm = RDONLY,
        .serial_number = "PCDEVLMO3333"
    },
    {
        .size = 32,
        .perm = WRONLY,
        .serial_number = "PCDEVPQR4444"
    }
};

//Create 2 platform devices

struct platform_device platform_pcdev1 = {
	.name = "pcdev-A1x",
	.id = 0,
    .dev = {
        .platform_data = &pcdev_pdata[0],
        .release = pcdev_release
    }
};

struct platform_device platform_pcdev2 = {
	.name = "pcdev-B1x",
	.id = 1,
    .dev = {
        .platform_data = &pcdev_pdata[1],
        .release = pcdev_release
    }
};

struct platform_device platform_pcdev3 = {
	.name = "pcdev-C1x",
	.id = 2,
    .dev = {
        .platform_data = &pcdev_pdata[2],
        .release = pcdev_release
    }
};

struct platform_device platform_pcdev4 = {
	.name = "pcdev-D1x",
	.id = 3,
    .dev = {
        .platform_data = &pcdev_pdata[3],
        .release = pcdev_release
    }
};

struct platform_device *pdev_array[] = {
    &platform_pcdev1,
    &platform_pcdev2,
    &platform_pcdev3,
    &platform_pcdev4
    };

static int __init pcdev_platform_init(void)
{
	//Register platform devices
    platform_add_devices(pdev_array, ARRAY_SIZE(pdev_array));

    pr_info("Device setup module loaded\n");
	
    return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev1);
	platform_device_unregister(&platform_pcdev2);
	platform_device_unregister(&platform_pcdev3);
	platform_device_unregister(&platform_pcdev4);
    pr_info("Device setup module unloaded\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amol");
MODULE_DESCRIPTION("Module which registers platform devices");
