# 1 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
# 1 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
# 2 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 2
# 3 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 2
# 4 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 2

// Pointer to stepper class member functions

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

  void render_back_menu_item(BackMenuItem const &menu_item) const { screen.print(menu_item.get_name()); }

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
    lcd_PROGMEM_to_buffer(1);
    screen.draw_render_menu(menu.get_name(), buffer);
  }
};
MyRenderer my_renderer;

// Forward declarations needed by MenuSystem
void spring_measurement();
void load_cell_calibration();
void lcd_print_force();

// Menu system which handles displayed text, call to functions via pointers, and
// numeric items (default, min, max, increment)
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
MenuItem menu3_5(" <  Print siste    ", &print_spring_const);

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
    // Encoder saturation
    // if(enc->value>100){enc->value=100;};
    // if(enc->value<0){enc->value=0;};

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
void lcd_PROGMEM_to_buffer(int index) { strcpy_P(buffer, (char *)
# 126 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
                                                                (__extension__({ uint16_t __addr16 = (uint16_t)((uint16_t)(
# 126 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                                                                &(string_table[index])
# 126 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
                                                                )); uint16_t __result; __asm__ __volatile__ ( "lpm %A0, Z+" "\n\t" "lpm %B0, Z" "\n\t" : "=r" (__result), "=z" (__addr16) : "1" (__addr16) ); __result; }))
# 126 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                                                                                                     ); }

// LOAD CELL #############################
void load_cell_init()
{
  scale.set_scale();
  scale.tare(); // Reset the scale to 0

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

void load_cell_tare() { scale.tare(); }

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
  lcd_PROGMEM_to_buffer(1);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK(); // wait for OK

  // Plasser vekt #
  screen.clear();
  lcd_PROGMEM_to_buffer(6);
  screen.write_text_line(0, 1, buffer);

  // Call set_scale() with no parameter, Call tare() with no parameter, no
  // interrupts
  
# 180 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
 __asm__ __volatile__ ("cli" ::: "memory")
# 180 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;
  scale.set_scale();
  scale.tare();
  
# 183 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
 __asm__ __volatile__ ("sei" ::: "memory")
# 183 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
             ;

  calibration_temp = load_cell.calibration_factor;
  calibration_factor[0] = calibration_temp;
  scale.set_scale(calibration_temp);

  for (int i = 1; i <= 3; i++)
  {
    // PLasser vekt # i |OK|
    screen.clear();
    lcd_PROGMEM_to_buffer(6);
    screen.write_text_int_line(0, 1, buffer, i);
    lcd_PROGMEM_to_buffer(1);
    screen.write_text_line(0, 3, buffer);
    button_block_until_OK();

    // Vekt: ###
    // Avlest | Kal.fak
    //  ###       ###
    screen.write_text_float_line(0, 0, "Vekt: ", weight[i]);
    screen.write_text_line(0, 1, "Avlest   |   Kal.fak");
    lcd_PROGMEM_to_buffer(2);
    screen.write_text_line(0, 3, buffer);

    
# 207 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("cli" ::: "memory")
# 207 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
    weight_read = scale.get_units();
    
# 209 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 209 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;
    weight_read_prev = weight_read;
    screen.write_float_float_line(2, weight_read, calibration_temp);

    while (!btn_OK.trig)
    {
      
# 215 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
     __asm__ __volatile__ ("cli" ::: "memory")
# 215 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                   ;
      scale.set_scale(calibration_temp);
      weight_read = scale.get_units(5); // average read
      
# 218 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
     __asm__ __volatile__ ("sei" ::: "memory")
# 218 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;

      // Display on screen if value has changed
      if (weight_read >= (weight_read_prev + 0.01) || weight_read <= (weight_read_prev - 0.01) ||
          calibration_temp != calibration_temp_prev)
      {
        screen.write_float_float_line(2, weight_read, calibration_temp);
      }
      weight_read_prev = weight_read;
      calibration_temp_prev = calibration_temp;

      if (weight_read > (weight[i] + 0.0001))
      {
        /*         if (weight_read > (weight[i] + 0.1))

        {

          calibration_temp += 10000.0;

        }

        else */
# 236 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
        if (weight_read > (weight[i] + 0.3))
        {
          calibration_temp += 300.0;
        }
        else if (weight_read > (weight[i] + 0.01))
        {
          calibration_temp += 200.0;
        }
        else if (weight_read > (weight[i] + 0.05))
        {
          calibration_temp += 100.0;
        }
        else
        {
          calibration_temp += 1.0;
        }
      }
      else if (weight_read < (weight[i] - 0.0001))
      {
        /*         if (weight_read < (weight[i] - 0.1))

        {

          calibration_temp -= 10000.0;

        }

        else  */
# 260 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
        if (weight_read < (weight[i] - 0.3))
        {
          calibration_temp -= 300.0;
        }
        else if (weight_read < (weight[i] - 0.01))
        {
          calibration_temp -= 200.0;
        }
        else if (weight_read < (weight[i] - 0.05))
        {
          calibration_temp -= 100.0;
        }
        else
        {
          calibration_temp -= 1.0;
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

      if (calibration_temp < 0)
      {
        calibration_temp = load_cell.calibration_factor;
      }

      button_get(4 /* Button Tilbake*/, &btn_TB);
      button_get(6 /* Button OK*/, &btn_OK);
    }

    screen.clear();

    // Kal.faktor ###
    // Vekt: ###
    // |OK|
    screen.write_text_float_line(0, 1, "Kal. faktor: ", calibration_temp);
    screen.write_text_float_line(0, 2, "Vekt: ", weight_read);
    lcd_PROGMEM_to_buffer(1);
    screen.write_text_line(0, 3, buffer);
    calibration_factor[i] = calibration_temp;
    button_block_until_OK();
  }

  // Find average
  for (int j = 0; j <= 3; j++)
  {
    calibration_avg += (calibration_factor[j]) / 4.0;
  }

  // Kalibrering ferdig
  // Kal. faktor: ###
  // |OK|
  screen.clear();
  screen.write_text_line(0, 0, "Kalibrering ferdig ");
  screen.write_text_float_line(0, 1, "Kal. faktor: ", calibration_temp);
  lcd_PROGMEM_to_buffer(1);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK();

  
# 337 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
 __asm__ __volatile__ ("cli" ::: "memory")
# 337 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;
  scale.set_scale(calibration_avg);
  
# 339 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
 __asm__ __volatile__ ("sei" ::: "memory")
# 339 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
             ;

  // Finished -> Back to menu
  ms.prev();
  ms.display();
}

// SPRING ################################
void spring_measurement()
{
  float min_kg = menu2_1.get_value();
  float max_kg = menu2_2.get_value();
  unsigned long step_per_check = (unsigned long)menu2_3.get_value();
  float weight_read, F = 0.0, x = 0.0;
  float const_k[64];

  // Spring must be attached and hanging loosely
  // Starter test |OK|
  screen.clear();
  screen.write_text_line(0, 1, "Starter test");
  lcd_PROGMEM_to_buffer(1);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK();

  
# 363 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
 __asm__ __volatile__ ("cli" ::: "memory")
# 363 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;
  scale.set_scale();
  scale.tare();
  scale.set_scale(load_cell.calibration_factor);
  
# 367 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
 __asm__ __volatile__ ("sei" ::: "memory")
# 367 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
             ;

  /*Spring measurement part 1

    Drive motor to minimum and set zero-point

    Three loops that move closer and closer

  */
# 374 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
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

    
# 391 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("cli" ::: "memory")
# 391 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
    weight_read = scale.get_units();
    
# 393 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 393 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;

    if (weight_read < min_kg)
    {
      motor.drive_distance(2, -1);
    }
    else
    {
      found_min_close = true;
    }

    delay(500);
    
# 405 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("cli" ::: "memory")
# 405 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
    weight_read = scale.get_units();
    
# 407 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 407 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;
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

    
# 424 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("cli" ::: "memory")
# 424 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
    weight_read = scale.get_units();
    
# 426 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 426 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;

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
    
# 442 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("cli" ::: "memory")
# 442 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
    weight_read = scale.get_units();
    
# 444 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 444 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;
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

    
# 462 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("cli" ::: "memory")
# 462 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
    weight_read = scale.get_units();
    
# 464 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 464 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;

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
    
# 480 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("cli" ::: "memory")
# 480 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
    weight_read = scale.get_units();
    
# 482 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 482 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;
    screen.write_float_line(3, 3, weight_read);
    delay(2500);
  }

  screen.clear();
  screen.write_text_line(0, 1, "Nullpunkt funnet");
  lcd_PROGMEM_to_buffer(1);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK();

  
# 493 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
 __asm__ __volatile__ ("cli" ::: "memory")
# 493 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;
  scale.tare();
  
# 495 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
 __asm__ __volatile__ ("sei" ::: "memory")
# 495 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
             ;

  /*Spring measurement part 2

    Calculate spring constants

    k = F/x                  */
# 500 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
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

      
# 523 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
     __asm__ __volatile__ ("cli" ::: "memory")
# 523 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                   ;
      weight_read = scale.get_units();
      
# 525 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
     __asm__ __volatile__ ("sei" ::: "memory")
# 525 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;

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

  float spring_const = 0.0f, spring_const_avg = 0.0f, spring_const_tot = 0.0f;
  for (int index = 0; index < i; index++)
  {
    spring_const_avg += const_k[index] / (float)i;

    Serial.print("spring_const_avg: ");
    Serial.println(spring_const_avg);
    Serial.print("k: ");
    Serial.print(index);
    Serial.print(" : ");
    Serial.println(const_k[index]);
  }
  // spring_const_avg = spring_const_tot / i;
  Serial.print("Final spring const: ");
  Serial.println(spring_const_avg);

  lcd_PROGMEM_to_buffer(7);
  screen.write_text_line(0, 0, buffer);
  screen.write_float_line(3, 1, spring_const_avg);
  lcd_PROGMEM_to_buffer(8);
  screen.write_text_line(0, 2, buffer);
  lcd_PROGMEM_to_buffer(1);
  screen.write_text_line(0, 3, buffer);
  button_block_until_OK();

  /* Spring measurement part 3

     Spring measured

     Releasing spring */
# 580 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
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

    
# 591 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("cli" ::: "memory")
# 591 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
    weight_read = scale.get_units();
    
# 593 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 593 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
               ;

    if (weight_read > 0.001)
    {
      motor.drive_up_10mm();
      delay(1000);

      
# 600 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
     __asm__ __volatile__ ("cli" ::: "memory")
# 600 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                   ;
      weight_read = scale.get_units();
      
# 602 "/home/albert/Documents/Stivhetssjekk/main/main.ino" 3
     __asm__ __volatile__ ("sei" ::: "memory")
# 602 "/home/albert/Documents/Stivhetssjekk/main/main.ino"
                 ;
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

// PRINTER
float spring_const_to_EEPROM(float x, float weight_read, int addr)
{
  // Store calculated spring constant [N/mm]
  EEPROM.write(addr, x);
}

void print_spring_const()
{
  for(int q = 0; q < 64; q++)
  {
    EEPROM.put(int(&spring[q].k), 39.93);
    EEPROM.put(int(&spring[q].F), 39.93);
    EEPROM.put(int(&spring[q].x), 39.93);
    Serial.print("in: ");
    Serial.println(q);
  }

  float k, f, x;
  for(int d = 0; d < 64; d++)
  {
    EEPROM.get(int(&spring[d].k), k);
    EEPROM.get(int(&spring[d].F), f);
    EEPROM.get(int(&spring[d].x), x);
    Serial.println(k);
    Serial.println(f);
    Serial.println(x);
    Serial.print("out: ");
    Serial.println(d);
  }


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
    case '-': // Previous item
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
    button_get(6 /* Button OK*/, &btn_OK);
    button_get(4 /* Button Tilbake*/, &btn_TB);

    btn_trigdelay = 0;
  }

  // Disable motor if Innstillinger pressed
  button_get(5 /* Button Innstillinger*/, &btn_IN);
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
  encoder_up = new ClickEncoder(14, 3);
  encoder_dn = new ClickEncoder(15, 2);

  // Buttons
  pinMode(6 /* Button OK*/, 0x0);
  pinMode(4 /* Button Tilbake*/, 0x0);
  pinMode(5 /* Button Innstillinger*/, 0x0);

  // lcd
  screen.begin();

  // Menu
  menu_init();

  Serial.println("HI");
  print_spring_const();
}

void loop()
{
  // Poll load cell
  load_cell_get();

  // Menu
  menu_handler();
}
