#include <Arduino.h>
#line 1 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
#line 1 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
#include "declarations.h"

// Pointer to stepper class member functions
// void (stepper::*up10) (void) = &stepper::drive_up_10mm;
// void (stepper::*dn10) (void) = &stepper::drive_down_10mm;
// void (stepper::*pwr) (void) = &stepper::poweron;
#line 7 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void up10();
#line 8 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void dn10();
#line 9 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void pwr();
#line 83 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
bool button_get(int BTN, btn_t *btn);
#line 99 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void button_block_until_OK();
#line 111 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void encoder_get(ClickEncoder *encoder, enc_t *enc);
#line 149 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void lcd_print_spring_const();
#line 157 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void lcd_PROGMEM_to_buffer(int index);
#line 163 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void load_cell_init();
#line 172 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void load_cell_get();
#line 182 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void load_cell_tare();
#line 344 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
float spring_const_to_EEPROM(float x, float weight_read, int addr);
#line 631 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void spring_measurement_quit();
#line 638 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
const String format_int(const float value);
#line 644 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
const String format_float(const float value);
#line 650 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void copy_to_dst(float *src, float *dst, int len);
#line 657 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void menu_init();
#line 678 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void menu_handler();
#line 762 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void timerIsr();
#line 794 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void setup();
#line 817 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void loop();
#line 7 "d:\\Dokumenter\\Stivhetssjekk\\main\\main.ino"
void up10() { motor.drive_up_10mm(); };
void dn10() { motor.drive_down_10mm(); };
void pwr() { motor.poweron(); };

// Menu renderer is a global class, do not move
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
    screen.draw_render_menu_item(menu_item.get_name(), buffer);
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
MyRenderer my_renderer;

// Forward declarations needed by MenuSystem
void spring_measurement();
void load_cell_calibration();
// void motor_drive_down_10mm();
// void motor_drive_up_10mm();
// void motor_poweron();
void lcd_print_force();

// Menu system which handles displayed text, call to functions via pointers, and numeric items (default, min, max, increment)
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
MenuItem menu3_4(" <  Aktiver motor >", &pwr);
NumericMenuItem menu3_5(" <      Kraft", &lcd_print_spring_const, 'f', 1, 1, 30, 1, format_int);

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
    // Encoder saturation
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
void lcd_print_spring_const()
{
  int i = menu3_5.get_value();
  float const_k = EEPROM.read(i);
  screen.write_text_float_line(0, 3, "Kraft: ", const_k);
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
  }
}

void load_cell_tare()
{
  scale.tare();
}

void load_cell_calibration()
{
  // Fetch weight values declared in menu
  float weight[5];
  weight[0] = 0.0;
  weight[1] = menu1_1.get_value();
  weight[2] = menu1_2.get_value();
  weight[3] = menu1_3.get_value();

  float weight_read, weight_read_prev;
  float calibration_temp, calibration_temp_prev;
  float calibration_factor[4];
  float calibration_avg = 0;

  // La vekten henge tom
  // Videre |OK|
  screen.clear();
  lcd_PROGMEM_to_buffer(5);
  screen.write_text_line(0, 1, buffer);
  lcd_PROGMEM_to_buffer(3);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK(); // wait for OK

  // Plasser vekt #
  screen.clear();
  lcd_PROGMEM_to_buffer(6);
  screen.write_text_line(0, 1, buffer);

  // Call set_scale() with no parameter, Call tare() with no parameter, no interrupts
  noInterrupts();
  scale.set_scale();
  scale.tare();
  interrupts();

  calibration_temp = load_cell.calibration_factor;
  calibration_factor[0] = calibration_temp;
  scale.set_scale(calibration_temp);

  for (int i = 1; i <= 3; i++)
  {
    // PLasser vekt # i |OK|
    screen.clear();
    lcd_PROGMEM_to_buffer(6);
    screen.write_text_int_line(0, 1, buffer, i);
    lcd_PROGMEM_to_buffer(3);
    screen.write_text_line(0, 3, buffer);
    button_block_until_OK();

    // Vekt: ###
    // Avlest | Kal.fak
    //  ###       ###
    screen.write_text_float_line(0, 0, "Vekt: ", weight[i]);
    screen.write_text_line(0, 1, "Avlest   |   Kal.fak");

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();
    weight_read_prev = weight_read;
    screen.write_float_float_line(2, weight_read, calibration_temp);

    while (!btn_OK.trig)
    {
      noInterrupts();
      scale.set_scale(calibration_temp);
      weight_read = scale.get_units();
      interrupts();

      // Display on screen if value has changed
      if (weight_read >= (weight_read_prev + 0.01) || weight_read <= (weight_read_prev - 0.01) || calibration_temp != calibration_temp_prev)
      {
        screen.write_float_float_line(2, weight_read, calibration_temp);
      }
      weight_read_prev = weight_read;
      calibration_temp_prev = calibration_temp;

      if (weight_read > weight[i] + 0.01)
      {
        if (weight_read > weight[i] + 0.1)
        {
          calibration_temp += 100;
        }
        else
        {
          calibration_temp += 10;
        }
      }
      else if (weight_read < weight[i] - 0.01)
      {
        if (weight_read < weight[i] + 0.1)
        {
          calibration_temp -= 100;
        }
        else
        {
          calibration_temp -= 10;
        }
      }

      // if (enc_up.clockwise < -1)
      // {
      //   calibration_temp -= 1000;
      //   enc_up.clockwise = 0;
      // }
      // if (enc_up.clockwise > 1)
      // {
      //   calibration_temp += 1000;
      //   enc_up.clockwise = 0;
      // }
      // if (enc_dn.clockwise < -1)
      // {
      //   calibration_temp -= 10;
      //   enc_dn.clockwise = 0;
      // }
      // if (enc_dn.clockwise > 1)
      // {
      //   calibration_temp += 10;
      //   enc_dn.clockwise = 0;
      // }

      button_get(BTN_OK, &btn_OK);
    }

    screen.clear();

    // Kal.faktor ###
    // Vekt: ###
    // |OK|
    screen.write_text_float_line(0, 1, "Kal. faktor: ", calibration_temp);
    screen.write_text_float_line(0, 2, "Vekt: ", weight_read);
    lcd_PROGMEM_to_buffer(3);
    screen.write_text_line(0, 3, buffer);
    calibration_factor[i] = calibration_temp;
    button_block_until_OK();
  }

  // Find average
  for (int j = 0; j <= 3; j++)
  {
    calibration_avg += (calibration_factor[j]) / 4.0;
  }

  screen.clear();
  screen.write_text_line(0, 0, "Kalibrering ferdig ");
  screen.write_text_float_line(0, 1, "Kal. faktor: ", calibration_temp);
  lcd_PROGMEM_to_buffer(3);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK();

  noInterrupts();
  scale.set_scale(calibration_avg);
  interrupts();

  ms.prev();
  ms.display();
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
  screen.clear();
  screen.write_text_line(0, 1, "Starter test");
  lcd_PROGMEM_to_buffer(3);
  screen.write_text_line(0, 3, buffer);
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
  screen.clear();
  screen.write_text_line(0, 0, "0-pkt:");
  screen.write_float_line(3, 1, min_kg);
  lcd_PROGMEM_to_buffer(9);
  screen.write_text_line(0, 2, buffer);
  while (!found_min_close)
  {
    // Safety, override motor
    if (motor.disabled)
    {
      spring_measurement_quit();
      return;
    }

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();

    if (weight_read < min_kg)
    {
      motor.drive_distance(2, -1);
    }
    else
    {
      found_min_close = true;
    }

    delay(500);
    noInterrupts();
    weight_read = scale.get_units();
    interrupts();
    screen.write_float_line(3, 3, weight_read);
    delay(500);
  }

  // Find accurate minimum
  lcd_PROGMEM_to_buffer(9);
  screen.write_text_line(0, 2, buffer);
  while (!found_min_closer)
  {
    // Safety, override motor
    if (motor.disabled)
    {
      spring_measurement_quit();
      return;
    }

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();

    if (weight_read < (min_kg - 0.01))
    {
      motor.drive_distance_accurate(5, -1);
    }
    else if (weight_read > (min_kg + 0.01))
    {
      motor.drive_distance_accurate(5, 1);
    }
    else
    {
      found_min_closer = true;
    }

    delay(500);
    noInterrupts();
    weight_read = scale.get_units();
    interrupts();
    screen.write_float_line(3, 3, weight_read);
    delay(1500);
  }

  lcd_PROGMEM_to_buffer(9);
  screen.write_text_line(0, 2, buffer);
  // Find perfect minimum
  while (!found_min_perfect)
  {
    // Safety, override motor
    if (motor.disabled)
    {
      spring_measurement_quit();
      return;
    }

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();

    if (weight_read < (min_kg - 0.001))
    {
      motor.drive_distance_accurate(1, -1);
    }
    else if (weight_read > (min_kg + 0.001))
    {
      motor.drive_distance_accurate(1, 1);
    }
    else
    {
      found_min_perfect = true;
    }

    delay(500);
    noInterrupts();
    weight_read = scale.get_units();
    interrupts();
    screen.write_float_line(3, 3, weight_read);
    delay(2500);
  }

  screen.clear();
  screen.write_text_line(0, 1, "Nullpunkt funnet");
  lcd_PROGMEM_to_buffer(3);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK();

  noInterrupts();
  scale.tare();
  interrupts();

  /*Spring measurement part 2 
    Calculate spring constants 
    k = F/x                  */
  bool found_max = false;
  int i = 0;

  screen.clear();
  screen.write_text_line(0, 0, "Maks vekt:");
  screen.write_float_line(3, 1, max_kg);
  lcd_PROGMEM_to_buffer(10);
  screen.write_text_line(0, 2, buffer);
  while (!found_max)
  {
    // Safety, override motor
    if (motor.disabled)
    {
      spring_measurement_quit();
      return;
    }

    if (weight_read < max_kg)
    {
      motor.drive_distance(step_per_check, -1);

      delay(1000);

      noInterrupts();
      weight_read = scale.get_units();
      interrupts();

      x += step_per_check;
      const_k[i] = (weight_read * CONST_g) / x;

      screen.write_float_float_line(3, weight_read, const_k[i]);
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

  screen.clear();

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
  screen.write_text_line(0, 0, buffer);
  screen.write_float_line(3, 1, spring_const_avg);
  lcd_PROGMEM_to_buffer(8);
  screen.write_text_line(0, 2, buffer);
  lcd_PROGMEM_to_buffer(3);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK();

  /* Spring measurement part 3
     Spring measured
     Releasing spring */
  bool finished_release = false;
  screen.clear();
  while (!finished_release)
  {
    // Safety, override motor
    if (motor.disabled)
    {
      spring_measurement_quit();
      return;
    }

    noInterrupts();
    weight_read = scale.get_units();
    interrupts();

    if (weight_read > 0.001)
    {
      motor.drive_up_10mm();
      delay(1000);

      noInterrupts();
      weight_read = scale.get_units();
      interrupts();
      screen.write_float_line(3, 3, weight_read);
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
    motor.shutdown();
    btn_IN.trig = 0;
  }
}

// SETUP AND LOOP ########################
void setup()
{
  Serial.begin(9600);

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  // Encoder
  encoder_up = new ClickEncoder(ENC_1A, ENC_1D);
  encoder_dn = new ClickEncoder(ENC_2A, ENC_2D);

  // Buttons
  pinMode(BTN_OK, INPUT);
  pinMode(BTN_TB, INPUT);
  pinMode(BTN_IN, INPUT);

  // lcd
  screen.begin();

  // Menu
  menu_init();
}

void loop()
{
  // Poll load cell
  load_cell_get();

  // Menu
  menu_handler();
}

