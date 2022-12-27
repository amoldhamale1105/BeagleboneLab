#include "lcd.h"
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

const struct attribute_group* lcd_attr_groups[] = {
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
    struct device* dev = &pdev->dev;
    struct lcd_private_data* dev_data = NULL;
    struct of_device_id* match = NULL;
    int i = 0, ret;

    pr_info("16x2 LCD detected\n");

    match = (struct of_device_id*)of_match_device(dev->driver->of_match_table, dev);
    if (match == NULL){
        dev_err(dev, "Connected LCD does not have a device tree config\n");
        return -EINVAL;
    }

    dev_data = (struct lcd_private_data*)devm_kzalloc(dev, sizeof(struct lcd_private_data), GFP_KERNEL);
    if (dev_data == NULL){
        dev_err(dev, "Can't allocate memory\n");
        return -ENOMEM;
    }

    mutex_init(&dev_data->lcd_lock);

    /* Save device private data for future use */
    dev->driver_data = (void*)dev_data;

    /* Allocate memory for 3 command gpio descriptor addresses (rs, rw, en) */
    dev_data->cmd_desc = (struct gpio_desc**)devm_kzalloc(dev, sizeof(struct gpio_desc*)*3, GFP_KERNEL);
    
    dev_data->cmd_desc[0] = devm_gpiod_get(dev, "rs", GPIOD_ASIS);
    dev_data->cmd_desc[1] = devm_gpiod_get(dev, "rw", GPIOD_ASIS);
    dev_data->cmd_desc[2] = devm_gpiod_get(dev, "en", GPIOD_ASIS);
    dev_data->data_descs = devm_gpiod_get_array(dev, "data", GPIOD_ASIS);
    
    if ((ret = gpiod_direction_output(dev_data->cmd_desc[0], 0))){
        dev_err(dev, "Direction setting failed for rs pin\n");
        return ret;
    }
    if ((ret = gpiod_direction_output(dev_data->cmd_desc[1], 0))){
        dev_err(dev, "Direction setting failed for rw pin\n");
        return ret;
    }
    if ((ret = gpiod_direction_output(dev_data->cmd_desc[2], 0))){
        dev_err(dev, "Direction setting failed for en pin\n");
        return ret;
    }

    for(i = 0; i < dev_data->data_descs->ndescs; i++)
    {
        if ((ret = gpiod_direction_output(dev_data->data_descs->desc[i], 0))){
            dev_err(dev, "Direction setting failed for D%d pin\n", i+4);
            return ret;
        }
    }
    
    drv_data.dev_lcd = device_create_with_groups(drv_data.class_lcd, dev, 0, dev_data, lcd_attr_groups, "LCD16x2");
    if(IS_ERR(drv_data.dev_lcd)){
        dev_err(dev, "Error in device creation\n");
        return PTR_ERR(drv_data.dev_lcd);
    }

    lcd_init(drv_data.dev_lcd);
    lcd_print_string(drv_data.dev_lcd, "16x2 LCD driver");
    
    return 0;
}

int lcd_driver_remove(struct platform_device* pdev)
{
    lcd_deinit(drv_data.dev_lcd);
    device_unregister(drv_data.dev_lcd);
    pr_info("LCD unregistered\n");
    return 0;
}

ssize_t lcdcmd_store(struct device* dev, struct device_attribute* dev_attr, const char* buf, size_t count)
{
    struct lcd_private_data* dev_data = (struct lcd_private_data*)dev_get_drvdata(dev);
    u8 cmd;
    ssize_t ret;
    
    mutex_lock(&dev_data->lcd_lock);
    ret = kstrtou8(buf, 0, &cmd);
    if (ret)
        goto out;

    if (cmd == LCD_CMD_DIS_CLEAR)
        lcd_display_clear(drv_data.dev_lcd);
    else if (cmd == LCD_CMD_DIS_RETURN_HOME)
        lcd_display_return_home(drv_data.dev_lcd);
    else
        lcd_send_command(drv_data.dev_lcd, cmd);
    ret = count;

out:
    mutex_unlock(&dev_data->lcd_lock);
    return ret;
}

ssize_t lcdscroll_store(struct device* dev, struct device_attribute* dev_attr, const char* buf, size_t count)
{
    struct lcd_private_data* dev_data = (struct lcd_private_data*)dev_get_drvdata(dev);
    ssize_t ret;

    mutex_lock(&dev_data->lcd_lock);
    if (sysfs_streq(buf, "on")){
        lcd_display_shift_left(drv_data.dev_lcd);
        ret = count;
    }
    else
        ret = -EINVAL;

    mutex_unlock(&dev_data->lcd_lock);
    return ret;
}

ssize_t lcdtext_store(struct device* dev, struct device_attribute* dev_attr, const char* buf, size_t count)
{
    struct lcd_private_data* dev_data = (struct lcd_private_data*)dev_get_drvdata(dev);

    mutex_lock(&dev_data->lcd_lock);
    lcd_print_string(drv_data.dev_lcd, buf);
    mutex_unlock(&dev_data->lcd_lock);

    return count;
}

ssize_t lcdxy_show(struct device* dev, struct device_attribute* dev_attr, char* buf)
{
    struct lcd_private_data* dev_data = (struct lcd_private_data*)dev_get_drvdata(dev);
    ssize_t ret;

    mutex_lock(&dev_data->lcd_lock);
    ret = sprintf(buf, "(%d,%d)\n", dev_data->cursor_pos[0], dev_data->cursor_pos[1]);
    mutex_unlock(&dev_data->lcd_lock);

    return ret;
}

ssize_t lcdxy_store(struct device* dev, struct device_attribute* dev_attr, const char* buf, size_t count)
{
    struct lcd_private_data* dev_data = (struct lcd_private_data*)dev_get_drvdata(dev);
    ssize_t ret;
    int xy_val;

    mutex_lock(&dev_data->lcd_lock);
    ret = kstrtoint(buf, 0, &xy_val);
    if (ret)
        goto out;
    if ((xy_val / 100 == 1) && (xy_val % 100 <= 40)){
        dev_data->cursor_pos[0] = 1;
        dev_data->cursor_pos[1] = xy_val % 100;
    }
    else if ((xy_val / 100 == 2) && (xy_val % 100 <= 40)){
        dev_data->cursor_pos[0] = 2;
        dev_data->cursor_pos[1] = xy_val % 100;
    }
    else if (xy_val / 10 == 1){
        dev_data->cursor_pos[0] = 1;
        dev_data->cursor_pos[1] = xy_val % 10;
    }
    else if (xy_val / 10 == 2){
        dev_data->cursor_pos[0] = 2;
        dev_data->cursor_pos[1] = xy_val % 10;
    }
    else{
        ret = -EINVAL;
        goto out;
    }

    lcd_set_cursor(drv_data.dev_lcd, dev_data->cursor_pos[0], dev_data->cursor_pos[1]);
    ret = count;
    
out:
    mutex_unlock(&dev_data->lcd_lock);
    return ret;
}