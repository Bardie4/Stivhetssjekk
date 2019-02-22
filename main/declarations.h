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
//#include <avr/pgmspace.h>
#include "LCD.h"
#include "stepper.h"

const char string_0[] PROGMEM = "<-:|Tilb.| Velg:|OK|"; // "String 0" etc are strings to store
const char string_1[] PROGMEM = "        Bekreft:|OK|";
const char string_2[] PROGMEM = "           Velg:|OK|";
const char string_3[] PROGMEM = "         Videre:|OK|";
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
//Adafruit_LiquidCrystal lcd(LCD_1, LCD_2, LCD_3);
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
const int btn_WAITTIME = 500;

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

// Motor
stepper motor(MOT_STEP, MOT_DIR, MOT_ENA);
// bool motor_disabled = false;

const float CONST_g = 9.807;

// Forward declarations
const String format_float(const float value);
const String format_int(const float value);
void lcd_PROGMEM_to_buffer(int index);
void load_cell_tare();
void spring_measurement();
void load_cell_calibration();
void lcd_print_force();
void lcd_print_spring_const();

// Pointer to stepper class member function
void up10() { motor.drive_up_10mm(); };
void dn10() { motor.drive_down_10mm(); };
void pwr() { motor.poweron(); };

// Menu render class
class MyRenderer : public MenuComponentRenderer
{
public:
  void render(Menu const &menu) const
  {
    screen.draw_render(menu.get_name());
    menu.get_current_component()->render(*this);
  }

  void render_menu_item(MenuItem const &menu_item) const
  {
    lcd_PROGMEM_to_buffer(0);
    screen.draw_render_menu_item(buffer, menu_item.get_name());
  }

  void render_back_menu_item(BackMenuItem const &menu_item) const
  {
    screen.print(menu_item.get_name());
  }

  void render_numeric_menu_item(NumericMenuItem const &menu_item) const
  {
    screen.draw_render_numeric_menu_item(menu_item.get_name(), menu_item.get_value());
    if (!menu_item.has_focus())
    {
      lcd_PROGMEM_to_buffer(0);
      screen.print(buffer);
    }
    else
    {
      lcd_PROGMEM_to_buffer(1);
      screen.print(buffer);
    }
  }

  void render_menu(Menu const &menu) const
  {
    lcd_PROGMEM_to_buffer(2);
    screen.draw_render_menu(menu.get_name(), buffer);
  }
};

// Menu renderer instance, set up as tree structure
MyRenderer my_renderer;

MenuSystem ms(my_renderer);
Menu menu1("      Kalibrer     >");
NumericMenuItem menu1_1("     Vekt#1 [kg]  >", nullptr, '1', 0.685, .100, 1.000, 0.10, format_float);
NumericMenuItem menu1_2(" <   Vekt#2 [kg]  >", nullptr, '2', 1.960, 1.500, 2.500, 0.10, format_float);
NumericMenuItem menu1_3(" <   Vekt#3 [kg]  >", nullptr, '3', 5.369, 4.500, 5.500, 0.10, format_float);
MenuItem menu1_4(" <     START", &load_cell_calibration);
Menu menu2("<  Stivhetssjekk   >");
NumericMenuItem menu2_1("   Min. kraft [kg]>", nullptr, '^', 0.0, 0.0, 5.0, 0.1, format_float);
NumericMenuItem menu2_2(" < Maks kraft [kg]>", nullptr, 'v', 0.1, 1.0, 5.0, 0.1, format_float);
NumericMenuItem menu2_3(" < steg/sjekk [mm]>", nullptr, 's', 1, 1, 10, 1, format_int);
MenuItem menu2_4(" <     START", &spring_measurement); // Start_measurement
Menu menu3("<  Manuell styring");
MenuItem menu3_1("   Motor opp 10mm >", &up10);
MenuItem menu3_2(" < Motor ned 10mm >", &dn10);
MenuItem menu3_3(" < Sett 0pkt last >", &load_cell_tare);
MenuItem menu3_4(" <  Aktiver motor", &pwr);
NumericMenuItem menu3_5(" <      Kraft     >", &lcd_print_spring_const, 'f', 1, 1, 30, 1, format_int);
