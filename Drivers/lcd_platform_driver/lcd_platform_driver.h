#ifndef LCD_PLATFORM_DRIVER_H
#define LCD_PLATFORM_DRIVER_H

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/types.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

int __init lcd_driver_init(void);
void __exit lcd_driver_exit(void);

int lcd_driver_probe(struct platform_device*);
int lcd_driver_remove(struct platform_device*);

// Attribute show and store functions
ssize_t lcdcmd_store(struct device*, struct device_attribute*, const char*, size_t);

ssize_t lcdscroll_store(struct device*, struct device_attribute*, const char*, size_t);

ssize_t lcdtext_store(struct device*, struct device_attribute*, const char*, size_t);

ssize_t lcdxy_show(struct device*, struct device_attribute*, char*);
ssize_t lcdxy_store(struct device*, struct device_attribute*, const char*, size_t);

//Device private data structure
struct lcd_private_data
{
    int cursor_pos[2];
    struct gpio_desc** cmd_desc;
    struct gpio_descs* data_descs;
    struct mutex lcd_lock;
};

//Driver private data structure
struct drv_private_data
{
    struct class *class_lcd;
    struct device *dev_lcd;
};

#endif