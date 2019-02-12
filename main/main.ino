/* 
  20 000 pulses at 2A & 1/4 microstep -> 25 cm
  -> 80 pulses per 1mm
*/

#include "Wire.h"
#include "Adafruit_LiquidCrystal.h" // "manage libraries"
#include <ClickEncoder.h>           // Dropbox
#include <TimerOne.h>               //"manage libraries"
#include "HX711.h"                  //https://github.com/bogde/HX711
#include "MenuSystem.h"
#include <EEPROM.h>
//#include <avr/pgmspace.h>

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

#define LCD_1 12
#define LCD_2 13
#define LCD_3 11

#define MM_TO_PULSE 80UL;
#define TENTH_MM_TO_PULSE 8UL;

char inChar;

// Screen ###########################
Adafruit_LiquidCrystal lcd(LCD_1, LCD_2, LCD_3);

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
bool motor_disabled = false;

const float CONST_g = 9.807;

// Forward declarations
const String format_float(const float value);
const String format_int(const float value);

class MyRenderer : public MenuComponentRenderer
{
public:
  void render(Menu const &menu) const
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(menu.get_name());
    lcd.setCursor(0, 1);
    menu.get_current_component()->render(*this);
  }

  void render_menu_item(MenuItem const &menu_item) const
  {
    lcd.print(menu_item.get_name());
    lcd.setCursor(0, 3);
    lcd_PROGMEM_to_buffer(0);
    lcd.print(buffer);
  }

  void render_back_menu_item(BackMenuItem const &menu_item) const
  {
    lcd.print(menu_item.get_name());
  }

  void render_numeric_menu_item(NumericMenuItem const &menu_item) const
  {
    lcd.print(menu_item.get_name());
    lcd.setCursor(0, 2);
    lcd.print("     ");
    lcd.print(menu_item.get_value());
    lcd.setCursor(0, 3);
    if (!menu_item.has_focus())
    {
      lcd_PROGMEM_to_buffer(0);
      lcd.print(buffer);
    }
    else
    {
      lcd_PROGMEM_to_buffer(1);
      lcd.print(buffer);
    }
  }

  void render_menu(Menu const &menu) const
  {
    lcd.print(menu.get_name());
    lcd.setCursor(0, 3);
    lcd_PROGMEM_to_buffer(2);
    lcd.print(buffer);
  }
};
MyRenderer my_renderer;

void spring_measurement();
void load_cell_calibration();
void motor_drive_down_10mm();
void motor_drive_up_10mm();
void load_cell_tare();
void motor_poweron();
void lcd_print_force();

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
MenuItem menu3_1("   Motor opp 10mm >", &motor_drive_up_10mm);
MenuItem menu3_2(" < Motor ned 10mm >", &motor_drive_down_10mm);
MenuItem menu3_3(" < Sett 0pkt last >", &load_cell_tare);
MenuItem menu3_4(" <  Aktiver motor", &motor_poweron);
NumericMenuItem menu3_5(" <      Kraft     >", &lcd_print_spring_const, 'f', 1, 1, 30, 1, format_int);

// ##################################

// BUTTONS ###############################

bool button_get(int BTN, btn_t *btn)
{
  btn->value = digitalRead(BTN);

  if (!btn->value)
  {
    btn->trig = true;
  }
  if (btn->value != btn->last)
  {
    btn->update = true;
  }

  btn->last = btn->value;
}

void button_block_until_OK()
{

  btn_OK.trig = false;
  while (!btn_OK.trig)
  {
    delay(10);
  }
  btn_OK.trig = false;
}

// ENCODERS ###############################

void encoder_get(ClickEncoder *encoder, enc_t *enc)
{
  enc->value += encoder->getValue();
  // if (enc->value < 0)
  // {
  //   if (enc->position > 20)
  //   {
  //     enc->position -= 20;
  //   }
  //   enc->position = (20 - abs(enc->value) % 20) % 20;
  // }
  // else
  // {
  //   enc->position = abs(enc->value) % 20;
  // }

  // Encoder sensitivity
  if (enc->value != enc->last)
  {
    // Enkoder saturation
    //if(enc->value>100){enc->value=100;};
    //if(enc->value<0){enc->value=0;};

    if (enc->value > enc->last)
    {
      enc->clockwise += 1;
    }
    else if (enc->value < enc->last)
    {
      enc->clockwise -= 1;
    }

    enc->last = enc->value;
    enc->update = true;
  }
}

// LCD ###################################

void lcd_write_text_int_line(int col, int row, String text, int val)
{
  lcd_clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(text);
  lcd.print(val);
}

void lcd_write_text_float_line(int col, int row, String text, float val)
{
  lcd_clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(text);
  lcd.print(val, 4);
}

void lcd_write_text_line(int col, int row, String text)
{
  lcd_clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(text);
}

void lcd_write_value_line(int col, int row, int val)
{
  lcd_clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(val);
}

void lcd_write_float_line(int col, int row, float val)
{
  lcd_clear_row(row);
  lcd.setCursor(col, row);
  lcd.print(val, 4);
}

void lcd_write_float_float_line(int row, float val_l, float val_r)
{
  lcd_clear_row(row);
  lcd.setCursor(0, row);
  lcd.print(val_l, 4);
  lcd.setCursor(10, row);
  lcd.print(val_r, 4);
}

void lcd_clear_row(int row)
{
  lcd.setCursor(0, row);
  strcpy_P(buffer, (char *)pgm_read_word(&(string_table[4])));
  lcd.print(buffer);
}

void lcd_clear_col(int col)
{
  for (int i = 0; i < 4; i++)
  {
    lcd.setCursor(col, i);
    lcd.print(" ");
  }
}

void lcd_print_spring_const()
{
  int i = menu3_5.get_value();
  float const_k = EEPROM.read(i);
  lcd_write_text_float_line(0, 3, "Kraft: ", const_k);
  delay(2000);
}

void lcd_PROGMEM_to_buffer(int index)
{
  strcpy_P(buffer, (char *)pgm_read_word(&(string_table[index])));
}

// LOAD CELL #############################
void load_cell_init()
{
  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  long zero_factor = scale.read_average();
  scale.set_scale(load_cell.calibration_factor);
}

void load_cell_get()
{
  if (load_cell.reading > 1000)
  {
    load_cell.value = scale.get_units();
    load_cell.reading = 0;
    load_cell.update = true;
    //serial.println(load_cell.value, 3);
  }
}

void load_cell_tare()
{
  scale.tare();
}

void load_cell_calibration()
{
  float weight[5];
  weight[0] = 0.0;
  weight[1] = menu1_1.get_value();
  weight[2] = menu1_2.get_value();
  weight[3] = menu1_3.get_value();

  float weight_read, weight_read_prev;

  float calibration_temp, calibration_temp_prev;
  float calibration_factor[4];
  float calibration_avg;

  lcd.clear();
  lcd_PROGMEM_to_buffer(5);
  lcd_write_text_line(0, 1, buffer);

  lcd_PROGMEM_to_buffer(3);
  lcd_write_text_line(0, 3, buffer);
  button_block_until_OK();
  lcd.clear();
  lcd_PROGMEM_to_buffer(6);
  lcd_write_text_line(0, 1, buffer);

  // Call set_scale() with no parameter.
  noInterrupts();
  scale.set_scale();
  // Call tare() with no parameter.
  scale.tare();
  interrupts();
  // Place a known weight on the scale and call get_units(10).

  calibration_temp = load_cell.calibration_factor;
  calibration_factor[0] = calibration_temp;
  scale.set_scale(calibration_temp);

  for (int i = 1; i <= 3; i++)
  {

    lcd.clear();
    lcd_PROGMEM_to_buffer(6);
    lcd_write_text_int_line(0, 1, buffer, i);

    lcd_PROGMEM_to_buffer(3);
    lcd_write_text_line(0, 3, buffer);
    button_block_until_OK();

    lcd_write_text_float_line(0, 0, "Vekt: ", weight[i]);
    lcd_write_text_line(0, 1, "Avlest   |   Kal.fak");

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();
    weight_read_prev = weight_read;
    lcd_write_float_float_line(2, weight_read, calibration_temp);

    while (!btn_OK.trig)
    {
      noInterrupts();
      scale.set_scale(calibration_temp);
      weight_read = scale.get_units();
      interrupts();

      if (weight_read >= (weight_read_prev + 0.01) || weight_read <= (weight_read_prev - 0.01) || calibration_temp != calibration_temp_prev)
      {
        lcd_write_float_float_line(2, weight_read, calibration_temp);
      }
      weight_read_prev = weight_read;
      calibration_temp_prev = calibration_temp;

      if (enc_up.clockwise < -1)
      {
        calibration_temp -= 1000;
        enc_up.clockwise = 0;
      }
      if (enc_up.clockwise > 1)
      {
        calibration_temp += 1000;
        enc_up.clockwise = 0;
      }
      if (enc_dn.clockwise < -1)
      {
        calibration_temp -= 10;
        enc_dn.clockwise = 0;
      }
      if (enc_dn.clockwise > 1)
      {
        calibration_temp += 10;
        enc_dn.clockwise = 0;
      }

      button_get(BTN_OK, &btn_OK);
    }

    lcd.clear();

    lcd_write_text_float_line(0, 1, "Kal. faktor: ", calibration_temp);
    lcd_write_text_float_line(0, 2, "Vekt: ", weight_read);
    lcd_PROGMEM_to_buffer(3);
    lcd_write_text_line(0, 3, buffer);
    calibration_factor[i] = calibration_temp;
    button_block_until_OK();
  }

  for (int j = 0; j <= 3; j++)
  {
    calibration_temp += (calibration_factor[j]);
  }
  calibration_temp /= 4;

  lcd.clear();
  lcd_write_text_line(0, 0, "Kalibrering ferdig ");
  lcd_write_text_float_line(0, 1, "Kal. faktor: ", calibration_temp);
  lcd_PROGMEM_to_buffer(3);
  lcd_write_text_line(0, 3, buffer);
  button_block_until_OK();

  noInterrupts();
  scale.set_scale(calibration_avg);
  interrupts();

  ms.prev();
  ms.display();
}

// MOTOR #################################
void motor_init()
{
  // Sets the two pins as Outputs
  pinMode(MOT_STEP, OUTPUT);
  pinMode(MOT_DIR, OUTPUT);
  pinMode(MOT_ENA, OUTPUT);

  digitalWrite(MOT_STEP, HIGH);
  digitalWrite(MOT_DIR, HIGH);
  digitalWrite(MOT_ENA, HIGH); // HIGH is off
}

void motor_enable()
{
  if (!motor_disabled)
  {
    digitalWrite(MOT_ENA, LOW); // LOW is on
  }
}

void motor_disable()
{
  digitalWrite(MOT_ENA, HIGH); // HIGH is off
}

void motor_poweron()
{
  motor_disabled = false;
}

void motor_shutdown()
{
  motor_disable();
  motor_disabled = true;
}

unsigned long motor_mm_to_pulses(unsigned long mm)
{
  return mm * MM_TO_PULSE;
}

unsigned long motor_tenth_mm_to_pulses(unsigned long tenth_mm)
{
  return tenth_mm * TENTH_MM_TO_PULSE;
}

void motor_drive_distance(unsigned long distance_mm, int direction)
{
  unsigned long pulses = motor_mm_to_pulses(distance_mm);

  motor_enable();

  if (direction == -1)
  {
    // Drive down
    digitalWrite(MOT_DIR, LOW);
    // Send the necessary amount of pulses
    for (unsigned long i = 0; i < pulses; i++)
    {
      digitalWrite(MOT_STEP, HIGH);
      delayMicroseconds(500);
      digitalWrite(MOT_STEP, LOW);
      delayMicroseconds(500);
    }
  }
  else if (direction == 1)
  {
    // Drive up
    digitalWrite(MOT_DIR, HIGH);
    // Send the necessary amount of pulses
    for (unsigned long i = 0; i < pulses; i++)
    {
      digitalWrite(MOT_STEP, HIGH);
      delayMicroseconds(500);
      digitalWrite(MOT_STEP, LOW);
      delayMicroseconds(500);
    }
  }

  motor_disable();
}

void motor_drive_distance_accurate(unsigned long distance_tenth_mm, int direction)
{
  unsigned long pulses = motor_tenth_mm_to_pulses(distance_tenth_mm);

  motor_enable();

  if (direction == -1)
  {
    // Drive down
    digitalWrite(MOT_DIR, LOW);
    // Send the necessary amount of pulses
    for (unsigned long i = 0; i < pulses; i++)
    {
      digitalWrite(MOT_STEP, HIGH);
      delayMicroseconds(500);
      digitalWrite(MOT_STEP, LOW);
      delayMicroseconds(500);
    }
  }
  else if (direction == 1)
  {
    // Drive up
    digitalWrite(MOT_DIR, HIGH);
    // Send the necessary amount of pulses
    for (unsigned long i = 0; i < pulses; i++)
    {
      digitalWrite(MOT_STEP, HIGH);
      delayMicroseconds(500);
      digitalWrite(MOT_STEP, LOW);
      delayMicroseconds(500);
    }
  }

  motor_disable();
}

void motor_drive_down_pulse()
{
  motor_enable();

  digitalWrite(MOT_DIR, LOW);

  digitalWrite(MOT_STEP, HIGH);
  delayMicroseconds(500);
  digitalWrite(MOT_STEP, LOW);
  delayMicroseconds(500);

  motor_disable();
}

void motor_drive_up_pulse()
{
  motor_enable();

  digitalWrite(MOT_DIR, HIGH);

  digitalWrite(MOT_STEP, HIGH);
  delayMicroseconds(500);
  digitalWrite(MOT_STEP, LOW);
  delayMicroseconds(500);

  motor_disable();
}

void motor_drive_down_10mm()
{
  motor_enable();

  unsigned long pulses = motor_mm_to_pulses(10);

  digitalWrite(MOT_DIR, LOW);
  for (unsigned long i = 0; i < pulses; i++)
  {
    digitalWrite(MOT_STEP, HIGH);
    delayMicroseconds(500);
    digitalWrite(MOT_STEP, LOW);
    delayMicroseconds(500);
  }

  motor_disable();
}

void motor_drive_up_10mm()
{
  motor_enable();

  unsigned long pulses = motor_mm_to_pulses(10);

  digitalWrite(MOT_DIR, HIGH);
  for (unsigned long i = 0; i < pulses; i++)
  {
    digitalWrite(MOT_STEP, HIGH);
    delayMicroseconds(500);
    digitalWrite(MOT_STEP, LOW);
    delayMicroseconds(500);
  }

  motor_disable();
}

// SPRING ################################
float spring_const_to_EEPROM(float x, float weight_read, int addr)
{
  // Store calculated spring constant [N/mm]
  float k;
  k = (weight_read * CONST_g) / (x);
  EEPROM.write(addr, k);
  return k;

  // if (k < 0.0)
  // {
  //   EEPROM.write(addr, 0.0);
  //   return 0;
  // }
  // else if (k > 1000.0)
  // {
  //   EEPROM.write(addr, 1000.0);
  //   return 1000;
  // }
  // else
  // {
  //   EEPROM.write(addr, k);
  //   return k;
  // }
}

void spring_measurement()
{
  float min_kg = menu2_1.get_value();
  float max_kg = menu2_2.get_value();
  unsigned long step_per_check = (unsigned long)menu2_3.get_value();
  float weight_read, F = 0.0, x = 0.0;
  float const_k[64];

  // Spring must be attached and hanging loosely
  lcd.clear();
  lcd_write_text_line(0, 1, "Starter test");
  lcd_PROGMEM_to_buffer(3);
  lcd_write_text_line(0, 3, buffer);
  button_block_until_OK();

  noInterrupts();
  scale.set_scale();
  scale.tare();
  scale.set_scale(load_cell.calibration_factor);
  interrupts();

  /*Spring measurement part 1
    Drive motor to minimum and set zero-point */

  bool found_min_close = false, found_min_closer = false, found_min_perfect = false;
  // Get close to minimum
  lcd.clear();
  lcd_write_text_line(0, 0, "0-pkt:");
  lcd_write_float_line(3, 1, min_kg);
  lcd_PROGMEM_to_buffer(9);
  lcd_write_text_line(0, 2, buffer);
  while (!found_min_close)
  {
    // Safety, override motor
    if (motor_disabled)
    {
      spring_measurement_quit();
      return;
    }

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();

    if (weight_read < min_kg)
    {
      motor_drive_distance(2, -1);
    }
    else
    {
      found_min_close = true;
    }

    delay(500);
    noInterrupts();
    weight_read = scale.get_units();
    interrupts();
    lcd_write_float_line(3, 3, weight_read);
    delay(500);
  }

  // Find accurate minimum
  lcd_PROGMEM_to_buffer(9);
  lcd_write_text_line(0, 2, buffer);
  while (!found_min_closer)
  {
    // Safety, override motor
    if (motor_disabled)
    {
      spring_measurement_quit();
      return;
    }

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();

    if (weight_read < (min_kg - 0.01))
    {
      motor_drive_distance_accurate(5, -1);
    }
    else if (weight_read > (min_kg + 0.01))
    {
      motor_drive_distance_accurate(5, 1);
    }
    else
    {
      found_min_closer = true;
    }

    delay(500);
    noInterrupts();
    weight_read = scale.get_units();
    interrupts();
    lcd_write_float_line(3, 3, weight_read);
    delay(1500);
  }

  lcd_PROGMEM_to_buffer(9);
  lcd_write_text_line(0, 2, buffer);
  // Find perfect minimum
  while (!found_min_perfect)
  {
    // Safety, override motor
    if (motor_disabled)
    {
      spring_measurement_quit();
      return;
    }

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();

    if (weight_read < (min_kg - 0.001))
    {
      motor_drive_distance_accurate(1, -1);
    }
    else if (weight_read > (min_kg + 0.001))
    {
      motor_drive_distance_accurate(1, 1);
    }
    else
    {
      found_min_perfect = true;
    }

    delay(500);
    noInterrupts();
    weight_read = scale.get_units();
    interrupts();
    lcd_write_float_line(3, 3, weight_read);
    delay(2500);
  }

  lcd.clear();
  lcd_write_text_line(0, 1, "Nullpunkt funnet");
  lcd_PROGMEM_to_buffer(3);
  lcd_write_text_line(0, 3, buffer);
  button_block_until_OK();

  noInterrupts();
  scale.tare();
  interrupts();

  /*Spring measurement part 2 
    Calculate spring constants 
    k = F/x                  */
  bool found_max = false;
  int i = 0;

  lcd.clear();
  lcd_write_text_line(0, 0, "Maks vekt:");
  lcd_write_float_line(3, 1, max_kg);
  lcd_PROGMEM_to_buffer(10);
  lcd_write_text_line(0, 2, buffer);
  while (!found_max)
  {
    // Safety, override motor
    if (motor_disabled)
    {
      spring_measurement_quit();
      return;
    }

    if (weight_read < max_kg)
    {
      motor_drive_distance(step_per_check, -1);

      delay(1000);

      noInterrupts();
      weight_read = scale.get_units();
      interrupts();

      x += step_per_check;
      const_k[i] = (weight_read * CONST_g) / x;

      lcd_write_float_float_line(3, weight_read, const_k[i]);
      Serial.print("x: ");
      Serial.println(x);
      Serial.print("weight_read: ");
      Serial.println(weight_read);
      Serial.print("i: ");
      Serial.println(i);
      Serial.print("const_k: ");
      Serial.println(const_k[i]);

      i++;
    }
    else
    {
      found_max = true;
    }

    delay(2000);
  }

  lcd.clear();

  float spring_const, spring_const_avg, spring_const_tot;
  for (int index = 0; index < i; index++)
  {
    spring_const_avg = const_k[index] / i;

    Serial.print("spring_const_tot: ");
    Serial.println(spring_const_avg);
    Serial.print("k: ");
    Serial.print(index);
    Serial.print(" : ");
    Serial.println(const_k[index]);
  }
  //spring_const_avg = spring_const_tot / i;
  Serial.print("spring_const_avg: ");
  Serial.println(spring_const_avg);

  lcd_PROGMEM_to_buffer(7);
  lcd_write_text_line(0, 0, buffer);
  lcd_write_float_line(3, 1, spring_const_avg);
  lcd_PROGMEM_to_buffer(8);
  lcd_write_text_line(0, 2, buffer);
  lcd_PROGMEM_to_buffer(3);
  lcd_write_text_line(0, 3, buffer);
  button_block_until_OK();

  /* Spring measurement part 3
     Spring measured
     Releasing spring */
  bool finished_release = false;
  lcd.clear();
  while (!finished_release)
  {
    // Safety, override motor
    if (motor_disabled)
    {
      spring_measurement_quit();
      return;
    }

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();

    if (weight_read > 0.001)
    {
      motor_drive_up_10mm();
      delay(1000);

      noInterrupts();
      weight_read = scale.get_units();
      interrupts();
      lcd_write_float_line(3, 3, weight_read);
    }
    else
    {
      finished_release = true;
    }
  }

  spring_measurement_quit();
}

void spring_measurement_quit()
{
  ms.prev();
  ms.display();
}

// MISC. #################################
const String format_int(const float value)
{
  // writes the (int) value of a float into a char buffer.
  return String((int)value);
}

const String format_float(const float value)
{
  // writes the value of a float into a char buffer.
  return String(value);
}

void copy_to_dst(float *src, float *dst, int len)
{
  // Function to copy 'len' elements from 'src' to 'dst'
  memcpy(dst, src, sizeof(src[0]) * len);
}

// MENU ##################################
void menu_init()
{
  ms.get_root_menu().add_menu(&menu1);
  menu1.add_item(&menu1_1);
  menu1.add_item(&menu1_2);
  menu1.add_item(&menu1_3);
  menu1.add_item(&menu1_4);
  ms.get_root_menu().add_menu(&menu2);
  menu2.add_item(&menu2_1);
  menu2.add_item(&menu2_2);
  menu2.add_item(&menu2_3);
  menu2.add_item(&menu2_4);
  ms.get_root_menu().add_menu(&menu3);
  menu3.add_item(&menu3_1);
  menu3.add_item(&menu3_2);
  menu3.add_item(&menu3_3);
  menu3.add_item(&menu3_4);
  menu3.add_item(&menu3_5);
  ms.display();
}

void menu_handler()
{
  // Handle Encoders
  if (enc_up.clockwise < -3)
  {
    inChar = 'w';
    enc_up.clockwise = 0;
  }
  if (enc_up.clockwise > 3)
  {
    inChar = 's';
    enc_up.clockwise = 0;
  }
  if (enc_dn.clockwise < -3)
  {
    inChar = '-';
    enc_dn.clockwise = 0;
  }
  if (enc_dn.clockwise > 3)
  {
    inChar = '+';
    enc_dn.clockwise = 0;
  }

  // Handle Buttons

  if (btn_OK.trig)
  {
    inChar = 'd';
    btn_OK.trig = false;
  }
  if (btn_TB.trig)
  {
    inChar = 'a';
    btn_TB.trig = false;
  }
  // if (btn_IN.trig)
  // {
  //   inChar = 'h';
  //   btn_IN.trig = false;
  // }

  if (inChar)
  {
    switch (inChar)
    {
    case 'w': // Previous item
      ms.prev();
      ms.display();
      inChar = 0;
      break;
    case 's': // Next item
      ms.next();
      ms.display();
      inChar = 0;
      break;
    case 'a': // Back presed
      ms.back();
      ms.display();
      inChar = 0;
      break;
    case 'd': // Select presed
      ms.select();
      ms.display();
      inChar = 0;
      break;
    case '+': // Next item
      ms.next_1();
      ms.display();
      inChar = 0;
      break;
    case '-': // Next item
      ms.prev_1();
      ms.display();
      inChar = 0;
      break;
    case '?':
    default:
      inChar = 0;
      break;
    }
  }
}

void timerIsr()
{
  encoder_up->service();
  encoder_dn->service();

  encoder_get(encoder_up, &enc_up);
  encoder_get(encoder_dn, &enc_dn);

  if (btn_trigdelay < btn_WAITTIME)
  {
    btn_trigdelay++;
  }

  if (btn_trigdelay >= btn_WAITTIME)
  {
    // Poll buttons
    button_get(BTN_OK, &btn_OK);
    button_get(BTN_TB, &btn_TB);

    btn_trigdelay = 0;
  }

  // Disable motor if Innstillinger pressed
  button_get(BTN_IN, &btn_IN);
  if (btn_IN.trig)
  {
    motor_shutdown();
    btn_IN.trig = 0;
  }
}


// SETUP AND LOOP ########################
void setup()
{
  Serial.begin(9600);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  // set up the LCD's number of rows and columns:
  lcd.begin(20, 3);

  // Encoder
  encoder_up = new ClickEncoder(ENC_1A, ENC_1D);
  encoder_dn = new ClickEncoder(ENC_2A, ENC_2D);

  // Buttons
  pinMode(BTN_OK, INPUT);
  pinMode(BTN_TB, INPUT);
  pinMode(BTN_IN, INPUT);

  // Load cell
  // load_cell_init();

  // Menu
  menu_init();

  // Motor
  motor_init();
}

void loop()
{
  // Poll load cell
  load_cell_get();

  // Menu
  menu_handler();

  // Motor
}
