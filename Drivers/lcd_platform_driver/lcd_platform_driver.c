#include "lcd_platform_driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amol Dhamale");
MODULE_DESCRIPTION("Platform driver for 16x2 character LCD");

module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

int __init lcd_driver_init(void)
{
    pr_info("lcd driver loaded successfully\n");
    return 0;
}

void __exit lcd_driver_exit(void)
{
    pr_info("lcd driver unloaded\n");
}