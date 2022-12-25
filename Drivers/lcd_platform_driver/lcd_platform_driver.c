#include "lcd_platform_driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amol Dhamale");
MODULE_DESCRIPTION("Platform driver for 16x2 character LCD");

module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

struct drv_private_data drv_data;

struct of_device_id match_table[] = {
    {.compatible = "org,lcd16x2"},
    {}
};

struct platform_driver lcd_driver = {
    .probe = lcd_driver_probe,
    .remove = lcd_driver_remove,
    .driver = {
        .name = "lcd16x2",
        .of_match_table = of_match_ptr(match_table)
    }
};

DEVICE_ATTR_WO(lcdcmd);
DEVICE_ATTR_WO(lcdtext);
DEVICE_ATTR_WO(lcdscroll);
DEVICE_ATTR_RW(lcdxy);

static struct attribute* lcd_attrs[] = {
    &dev_attr_lcdcmd.attr,
    &dev_attr_lcdtext.attr,
    &dev_attr_lcdscroll.attr,
    &dev_attr_lcdxy.attr,
    NULL
};

static struct attribute_group lcd_attr_group = {
    .attrs = lcd_attrs
};

struct attribute_group* lcd_attr_groups[] = {
    &lcd_attr_group,
    NULL
};

int __init lcd_driver_init(void)
{
    drv_data.class_lcd = class_create(THIS_MODULE, "lcd");
    if(IS_ERR(drv_data.class_lcd)){
        pr_err("Error in creating class\n");
        return PTR_ERR(drv_data.class_lcd);
    }
    platform_driver_register(&lcd_driver);
    pr_info("lcd driver loaded successfully\n");
    return 0;
}

void __exit lcd_driver_exit(void)
{
    platform_driver_unregister(&lcd_driver);
    class_destroy(drv_data.class_lcd);
    pr_info("lcd driver unloaded\n");
}

int lcd_driver_probe(struct platform_device* pdev)
{
    return 0;
}

int lcd_driver_remove(struct platform_device* pdev)
{
    return 0;
}

ssize_t lcdcmd_store(struct device* dev, struct device_attribute* dev_attr, const char* buf, size_t count)
{
    return 0;
}

ssize_t lcdscroll_store(struct device* dev, struct device_attribute* dev_attr, const char* buf, size_t count)
{
    return 0;
}

ssize_t lcdtext_store(struct device* dev, struct device_attribute* dev_attr, const char* buf, size_t count)
{
    return 0;
}

ssize_t lcdxy_show(struct device* dev, struct device_attribute* dev_attr, char* buf)
{
    return 0;
}

ssize_t lcdxy_store(struct device* dev, struct device_attribute* dev_attr, const char* buf, size_t count)
{
    return 0;
}