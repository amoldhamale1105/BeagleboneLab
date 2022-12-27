#include <linux/delay.h>
#include "lcd.h"
#include "lcd_platform_driver.h"

void gpio_write_value(struct device* dev, int pin_type, int index, u8 out_value)
{
	struct gpio_desc* target_desc = NULL;
	struct lcd_private_data* dev_data = (struct lcd_private_data*)dev_get_drvdata(dev);
	
	target_desc = pin_type == LCD_CMD ? dev_data->cmd_desc[index] : dev_data->data_descs->desc[index];

	if ((int)out_value <= HIGH_VALUE)
		gpiod_set_value(target_desc, (int)out_value);
}

void lcd_deinit(struct device* dev)
{
	lcd_display_clear(dev);      	  /* Clear display */
	lcd_display_return_home(dev);        /* Cursor at home position */
}

/* 
 * LCD initalization sequence . 
 * This is written as per the HITACHI HD44780U datasheet 
 * https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
 * Ref. page number 46 , Figure 24 for intialization sequence for 4-bit interface 
 */
void lcd_init(struct device* dev)
{
	mdelay(40);
	
	/* RS=0 for LCD command */
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_RS, LOW_VALUE);
	
	/*R/nW = 0, for write */
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_RW, LOW_VALUE);
	
	write_4_bits(dev, 0x03);
	mdelay(5);
	
	write_4_bits(dev, 0x03);
	mdelay(1);
	
	write_4_bits(dev, 0x03);
	write_4_bits(dev, 0x02);

    /*4 bit data mode, 2 lines selection , font size 5x8 */
	lcd_send_command(dev, LCD_CMD_4DL_2N_5X8F);
	
	/* Display ON, Cursor ON */
	lcd_send_command(dev, LCD_CMD_DON_CURON);
	
	lcd_display_clear(dev);
	
	/*Address auto increment*/
	lcd_send_command(dev, LCD_CMD_INCADD);	
}

/*Clear the display */
void lcd_display_clear(struct device* dev)
{
	struct lcd_private_data* dev_data = (struct lcd_private_data*)dev_get_drvdata(dev);
	lcd_send_command(dev, LCD_CMD_DIS_CLEAR);
	/*
	 * check page number 24 of datasheet.
	 * display clear command execution wait time is around 2ms
	 */
	mdelay(2);
	dev_data->cursor_pos[0] = 1;
	dev_data->cursor_pos[1] = 1;
}

/*Cursor returns to home position */
void lcd_display_return_home(struct device* dev)
{
	struct lcd_private_data* dev_data = (struct lcd_private_data*)dev_get_drvdata(dev);
	lcd_send_command(dev, LCD_CMD_DIS_RETURN_HOME);
	/*
	 * check page number 24 of datasheet.
	 * return home command execution wait time is around 2ms
	 */
	mdelay(2);
	dev_data->cursor_pos[0] = 1;
	dev_data->cursor_pos[1] = 1;
}

/**
 * @brief Shift LCD display to the left by one position
 * 
 */
void lcd_display_shift_left(struct device* dev)
{
	lcd_send_command(dev, LCD_CMD_DIS_SHIFT_LEFT);
	mdelay(1);
}

/**
  * @brief  Set Lcd to a specified location given by row and column information
  * @param  Row Number (1 to 2)
  * @param  Column Number (1 to 16) Assuming a 2 X 16 characters display
  */
void lcd_set_cursor(struct device* dev, u8 row, u8 column)
{
	column--;
	switch (row)
	{
		case 1:
			/* Set cursor to 1st row address and add index*/
			lcd_send_command(dev, column |= DDRAM_FIRST_LINE_BASE_ADDR);
		break;
		case 2:
			/* Set cursor to 2nd row address and add index*/
			lcd_send_command(dev, column |= DDRAM_SECOND_LINE_BASE_ADDR);
		break;
		default:
		break;
	}
}

/* writes 4 bits of data/command on to D4,D5,D6,D7 lines */
void write_4_bits(struct device* dev, u8 data)
{
	/* 4 bits parallel data write */
	gpio_write_value(dev, LCD_DATA, GPIO_LCD_D4, (data >> 0 ) & 0x1);
	gpio_write_value(dev, LCD_DATA, GPIO_LCD_D5, (data >> 1 ) & 0x1);
	gpio_write_value(dev, LCD_DATA, GPIO_LCD_D6, (data >> 2 ) & 0x1);
	gpio_write_value(dev, LCD_DATA, GPIO_LCD_D7, (data >> 3 ) & 0x1);
	
	lcd_enable(dev);
	
}
/*
 * @brief call this function to make LCD latch the data lines in to its internal registers.
 */
void lcd_enable(struct device* dev)
{ 
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_EN, LOW_VALUE);
	mdelay(1);
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_EN, HIGH_VALUE);
	mdelay(1);
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_EN, LOW_VALUE);
	mdelay(1); /* execution time > 37 micro seconds */
}

/*
 *This function sends a character to the LCD 
 *Here we used 4 bit parallel data transmission. 
 *First higher nibble of the data will be sent on to the data lines D4,D5,D6,D7
 *Then lower niblle of the data will be set on to the data lines D4,D5,D6,D7
 */
void lcd_print_char(struct device* dev, char data)
{
	//RS=1, for user data
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_RS, HIGH_VALUE);
	
	/*R/nW = 0, for write */
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_RW, LOW_VALUE);
	
	write_4_bits(dev, data >> 4); /* higher nibble */
	write_4_bits(dev, data);      /* lower nibble */
}

void lcd_print_string(struct device* dev, const char *message)
{
	do
	{
		lcd_print_char(dev, (char)*message++);
	}
	while (*message != '\0');

}

/*
 *This function sends a command to the LCD 
 */
void lcd_send_command(struct device* dev, u8 command)
{
	/* RS=0 for LCD command */
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_RS, LOW_VALUE);
	
	/*R/nW = 0, for write */
	gpio_write_value(dev, LCD_CMD, GPIO_LCD_RW, LOW_VALUE);
	
	write_4_bits(dev, command >> 4); /* higher nibble */
	write_4_bits(dev, command);     /* lower nibble */

}

void lcd_printf(struct device* dev, const char *fmt, ...)
{
	int i;
	uint32_t text_size, letter;
      static char text_buffer[32];
	va_list args;

	va_start(args, fmt);
	text_size = vsprintf(text_buffer, fmt, args);

	// Process the string
	for (i = 0; i < text_size; i++)
	{
	letter = text_buffer[i];

	if (letter == 10)
		break;
	else
	{
		if ((letter > 0x1F) && (letter < 0x80))
			lcd_print_char(dev, letter);
	}
	}
}
