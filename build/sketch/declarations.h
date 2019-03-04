/* 
  20 000 pulses at 2A & 1/4 microstep -> 25 cm
  -> 80 pulses per 1mm
*/

#include "Wire.h"
#include "Adafruit_LiquidCrystal.h" // "manage libraries"
#include <ClickEncoder.h>           // Dropbox
#include <TimerOne.h>               // "manage libraries"
#include "HX711.h"                  // https://github.com/bogde/HX711
#include "MenuSystem.h"             // edited version of https://github.com/jonblack/arduino-menusystem
#include <EEPROM.h>
#include <avr/eeprom.h>
//#include <avr/pgmspace.h>
#include "LCD.h"
#include "stepper.h"

const char string_0[] PROGMEM = "|TB|            |OK|"; // "String 0" etc are strings to store
const char string_1[] PROGMEM = "                |OK|";
const char string_2[] PROGMEM = "                |OK|";
const char string_3[] PROGMEM = " ";
const char string_4[] PROGMEM = "                    ";
const char string_5[] PROGMEM = "La vekten henge tom";
const char string_6[] PROGMEM = "Plasser vekt #";
const char string_7[] PROGMEM = "Stivhet k = ";
const char string_8[] PROGMEM = "Neste: slipper fjaer";
const char string_9[] PROGMEM = "Vekt[kg]";
const char string_10[] PROGMEM = "Vekt[kg]|Konst[N/mm]";

const char *const string_table[] PROGMEM = {string_0, string_1, string_2, string_3, string_4, string_5, string_6, string_7, string_8, string_9, string_10};

char buffer[21];

#define MOT_STEP 19 // Motor step (+)
#define MOT_DIR 18  // Motor direction (+)
#define MOT_ENA 17  // Motor enable (+)

#define BTN_OK 6 // Button OK
#define BTN_TB 4 // Button Tilbake
#define BTN_IN 5 // Button Innstillinger

#define LC_DOUT 7 // Load Cell Data
#define LC_CLK 8  // Load Cell CLK input

#define ENC_1A 14
#define ENC_1D 3
#define ENC_2A 15
#define ENC_2D 2

char inChar;

// Screen ###########################
LCD screen;

// Encoders #########################
ClickEncoder *encoder_up, *encoder_dn;

typedef struct
{
  int last = -1, value, clockwise; // position
  bool update = true;
} enc_t;

enc_t enc_up, enc_dn;

// Buttons ##########################
typedef struct
{
  bool value = false, last = false, update = true, trig = false;
} btn_t;

int btn_trigdelay;
const int btn_WAITTIME = 700;

btn_t btn_OK, btn_TB, btn_IN;

// Load-cell ########################
HX711 scale(LC_DOUT, LC_CLK);

struct scl_t
{
  float value;
  int reading;
  bool update = true;
  float calibration_factor = 43230.0;
} load_cell;

// Motor ############################
stepper motor(MOT_STEP, MOT_DIR, MOT_ENA);

const float CONST_g = 9.807;

// Spring ###########################
typedef struct {
  float k; // 4 byte
  float F; // 4 byte
  float x; // 4 byte
} spring_type; // 13 byte

spring_type spring[64] EEMEM;

// Forward declarations
const String format_float(const float value);
const String format_int(const float value);
void lcd_PROGMEM_to_buffer(int index);
void load_cell_tare();
void spring_measurement();
void load_cell_calibration();
void print_spring_const();

// Pointer to stepper class member function
void up10() { motor.drive_up_10mm(); };
void dn10() { motor.drive_down_10mm(); };
void pwr() { motor.poweron(); };
