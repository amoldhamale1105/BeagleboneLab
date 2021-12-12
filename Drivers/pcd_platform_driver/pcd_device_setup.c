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
struct pcdev_platform_data pcdev_pdata[2] = {
    {
        .size = 512,
        .perm = RDWR,
        .serial_number = "PCDEVABC1111"
    },
    {
        .size = 1024,
        .perm = RDWR,
        .serial_number = "PCDEVXYZ2222"
    }
};

//Create 2 platform devices

struct platform_device platform_pcdev1 = {
	.name = "pseudo-char-device",
	.id = 0,
    .dev = {
        .platform_data = &pcdev_pdata[0],
        .release = pcdev_release
    }
};

struct platform_device platform_pcdev2 = {
	.name = "pseudo-char-device",
	.id = 1,
    .dev = {
        .platform_data = &pcdev_pdata[1],
        .release = pcdev_release
    }
};

static int __init pcdev_platform_init(void)
{
	//Register platform device
	platform_device_register(&platform_pcdev1);
	platform_device_register(&platform_pcdev2);

    pr_info("Device setup module loaded\n");
	
    return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev1);
	platform_device_unregister(&platform_pcdev2);
    pr_info("Device setup module unloaded\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module which registers platform devices");
