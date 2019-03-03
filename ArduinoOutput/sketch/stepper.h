/*
  LCD.h - Library for using Adafruit_LiquidCrystal screen with Wire
  Created by Albert Danielsen, November 18 2019
*/

#ifndef stepper_h
#define stepper_h

#include "Arduino.h"

#define LCD_1 12
#define LCD_2 13
#define LCD_3 11

#define MM_TO_PULSE 80UL;
#define TENTH_MM_TO_PULSE 8UL;

class stepper
{
  public:
    int STEP, DIR, ENA;
    bool disabled = false;

    stepper(int, int, int);
    void enable();
    void disable();
    void poweron();
    void shutdown();
    unsigned long mm_to_pulses(unsigned long mm);
    unsigned long tenth_mm_to_pulses(unsigned long tenth_mm);
    void drive_distance(unsigned long distance_mm, int direction);
    void drive_distance_accurate(unsigned long distance_tenth_mm, int direction);
    void drive_down_pulse();
    void drive_up_pulse();
    void drive_down_10mm();
    void drive_up_10mm();
};

#endif
