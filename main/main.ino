#include "declarations.h"

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

  // Encoder sensitivity
  if (enc->value != enc->last)
  {

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

  // 
  screen.clear();
  lcd_PROGMEM_to_buffer(5);
  screen.write_text_line(0, 1, buffer);

  lcd_PROGMEM_to_buffer(3);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK();
  screen.clear();
  lcd_PROGMEM_to_buffer(6);
  screen.write_text_line(0, 1, buffer);

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

    screen.clear();
    lcd_PROGMEM_to_buffer(6);
    screen.write_text_int_line(0, 1, buffer, i);

    lcd_PROGMEM_to_buffer(3);
    screen.write_text_line(0, 3, buffer);
    button_block_until_OK();

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

      if (weight_read >= (weight_read_prev + 0.01) || weight_read <= (weight_read_prev - 0.01) || calibration_temp != calibration_temp_prev)
      {
        screen.write_float_float_line(2, weight_read, calibration_temp);
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

    screen.clear();

    screen.write_text_float_line(0, 1, "Kal. faktor: ", calibration_temp);
    screen.write_text_float_line(0, 2, "Vekt: ", weight_read);
    lcd_PROGMEM_to_buffer(3);
    screen.write_text_line(0, 3, buffer);
    calibration_factor[i] = calibration_temp;
    button_block_until_OK();
  }

  for (int j = 0; j <= 3; j++)
  {
    calibration_temp += (calibration_factor[j]);
  }
  calibration_temp /= 4;

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
  // float -> int -> char buffer
  return String((int)value);
}

const String format_float(const float value)
{
  // float -> char buffer
  return String(value);
}

void copy_to_dst(float *src, float *dst, int len)
{
  // copy 'len' elements from 'src' to 'dst'
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
