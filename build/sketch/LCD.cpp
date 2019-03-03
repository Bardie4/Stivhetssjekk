/*
  LCD.cpp - Library for using Adafruit_LiquidCrystal screen with Wire
  Created by Albert Danielsen, November 18 2019
*/

#include "LCD.h"
#include "Arduino.h"
#include "Adafruit_LiquidCrystal.h"
#include "Wire.h"
#include <EEPROM.h>

Adafruit_LiquidCrystal lcd(LCD_1, LCD_2, LCD_3);

LCD::LCD()
{
}

void LCD::begin()
{
  // set up the LCD's number of rows and columns:
  lcd.begin(20, 3);
}

void LCD::print(String c)
{
  lcd.print(c);
}

void LCD::clear()
{
  lcd.clear();
}

void LCD::write_text_int_line(int col, int row, String text, int val)
{
  this->clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(text);
  lcd.print(val);
}

void LCD::write_text_float_line(int col, int row, String text, float val)
{
  this->clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(text);
  lcd.print(val, 4);
}

void LCD::write_text_line(int col, int row, String text)
{
  this->clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(text);
}

void LCD::write_value_line(int col, int row, int val)
{
  this->clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(val);
}

void LCD::write_float_line(int col, int row, float val)
{
  this->clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(val, 4);
}

void LCD::write_float_float_line(int row, float val_l, float val_r)
{
  this->clear_row(row);
  lcd.setCursor(0, row);
  lcd.print(val_l, 4);
  lcd.setCursor(10, row);
  lcd.print(val_r, 4);
}

void LCD::clear_row(int row)
{
  lcd.setCursor(0, row);
  lcd.print("                    ");
}

void LCD::clear_col(int col)
{
  for (int i = 0; i < 4; i++)
  {
    lcd.setCursor(col, i);
    lcd.print(" ");
  }
}

void LCD::draw_render(String name)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(name);
  lcd.setCursor(0, 1);
}

void LCD::draw_render_menu_item(String name, const char *c)
{
  lcd.setCursor(0, 1);
  lcd.print(name);
  lcd.setCursor(0, 3);
  lcd.print(c);
}

void LCD::draw_render_numeric_menu_item(String name, float f)
{
  lcd.print(name);
  lcd.setCursor(0, 2);
  lcd.print("     ");
  lcd.print(f);
  lcd.setCursor(0, 3);
}

void LCD::draw_render_menu(String name, const char *c)
{
  lcd.setCursor(0, 0);
  lcd.print(name);
  lcd.setCursor(0, 3);
  lcd.print(c);
}
