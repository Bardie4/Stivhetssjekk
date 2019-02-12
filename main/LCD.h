/*
  LCD.h - Library for using Adafruit_LiquidCrystal screen with Wire
  Created by Albert Danielsen, November 18 2019
*/

#ifndef LCD_h
#define LCD_h

#include "Arduino.h"
#include "Adafruit_LiquidCrystal.h"
#include "Wire.h"
#include <EEPROM.h>

class LCD
{
  public:
    LCD();
    void print(String c);
    void clear();
    void write_text_int_line(int col, int row, String text, int val);
    void write_text_float_line(int col, int row, String text, float val);
    void write_text_line(int col, int row, String text);
    void write_value_line(int col, int row, int val);
    void write_float_line(int col, int row, float val);
    void write_float_float_line(int row, float val_l, float val_r);
    void clear_row(int row);
    void clear_col(int col);
    void print_spring_const();
    void PROGMEM_to_buffer(int index);
    void LCD::draw_render(String name);
    void LCD::draw_render_menu_item(String name, const char * c);
    void LCD::draw_render_numeric_menu_item(String name, float f);
    void LCD::draw_render_menu(String name, const char * c);
};

#endif

