#include <linux/types.h>

#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#define LCD_CMD 0
#define LCD_DATA 1

#define GPIO_LCD_RS   0   /*  Register selection (Character vs. Command) */ 
#define GPIO_LCD_RW   1   /*  Read/write     */
#define GPIO_LCD_EN   2   /*  Enable */
#define GPIO_LCD_D4   0   /*  Data line 4    */
#define GPIO_LCD_D5   1   /*  Data line 5    */
#define GPIO_LCD_D6   2   /*  Data line 6    */
#define GPIO_LCD_D7   3   /*  Data line 7    */

/*LCD commands */
#define LCD_CMD_4DL_2N_5X8F  		0x28
#define LCD_CMD_DON_CURON    		0x0E
#define LCD_CMD_INCADD       		0x06
#define LCD_CMD_DIS_CLEAR    		0X01
#define LCD_CMD_DIS_RETURN_HOME  	0x02
#define LCD_CMD_DIS_SHIFT_LEFT      0x18


/*Sets CGRAM address. CGRAM data is sent and received after this setting. */
#define LCD_CMD_SET_CGRAM_ADDRESS  			0x40

/* Sets DDRAM address. DDRAM data is sent and received after this setting. */
#define LCD_CMD_SET_DDRAM_ADDRESS  			0x80

#define DDRAM_SECOND_LINE_BASE_ADDR         	(LCD_CMD_SET_DDRAM_ADDRESS | 0x40 )
#define DDRAM_FIRST_LINE_BASE_ADDR          	LCD_CMD_SET_DDRAM_ADDRESS

#define LCD_ENABLE 1
#define LCD_DISABLE 0

#define HIGH_VALUE 1
#define LOW_VALUE 0

struct device;

//public function prototypes
void lcd_deinit(struct device*);
void lcd_init(struct device*);
void lcd_set_cursor(struct device*, u8 row, u8 column);
void lcd_enable(struct device*);
void lcd_print_char(struct device*, char ascii_Value);
void lcd_print_string(struct device*, const char *message);
void lcd_send_command(struct device*, u8 command);
void lcd_display_clear(struct device*);
void lcd_printf(struct device*, const char *fmt, ...);
void lcd_display_return_home(struct device*);
void lcd_display_shift_left(struct device*);
void write_4_bits(struct device*, u8 data);

//gpio access methods
void gpio_write_value(struct device*, int pin_type, int index, u8 out_value);

#endif /* LCD_DRIVER_H_ */
