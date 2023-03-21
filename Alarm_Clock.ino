// Alarm_Clock on Arduino Uno//

#include <LiquidCrystal.h> //Libraries attached
#define PRESCALER2 7812    //"User Prescaler"
#define potentiometer A0
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // PINs LCD are connected
uint8_t time_sec = 0, time_min = 0, time_h = 0, one_sec = 0;
volatile uint8_t state = 0;

void lcd_print_time(uint8_t(*time_sec), uint8_t(*time_min), uint8_t(*time_h))
{
  if ((*time_sec) & (1 << 7))
  {
    (*time_sec) = ~(1 << 7) & (*time_sec);
    if ((*time_sec) >= 60)
    {
      (*time_sec) = 0;
      (*time_min)++;
      (*time_min) = (1 << 7) | (*time_min); // 8th bit are used to do once change displaing value on LCD
    }
    lcd.setCursor(10, 1);
    if ((*time_sec) < 10)
      lcd.print("0");
    lcd.print(*time_sec);
  }
  if ((*time_min) & (1 << 7))
  {
    (*time_min) = ~(1 << 7) & (*time_min);
    if ((*time_min) >= 60)
    {
      (*time_min) = 0;
      (*time_h)++;
      (*time_h) = (1 << 7) | (*time_h); // 8th bit are used to do once change displaing value on LCD
    }
    lcd.setCursor(7, 1);
    if ((*time_min) < 10)
      lcd.print("0");
    lcd.print(*time_min);
  }
  if ((*time_h) & (1 << 7))
  {
    (*time_h) = ~(1 << 7) & (*time_h);
    if ((*time_h) >= 23)
      (*time_h) = 0;
    lcd.setCursor(4, 1);
    if ((*time_h) < 10)
      lcd.print("0");
    lcd.print(*time_h);
  }
}

void lcd_time_set_by_serial(uint8_t(*time_h), uint8_t(*time_min))
{
  Serial.print("Set hour: ");
  uint8_t time_h2[2] = {0x00, 0x00};
  while ((time_h2[0] == 0x00) & (time_h2[1] == 0x00)) // loop executed until the numbers are in the buffer
  {
    if (Serial.available() > 0) // if receive any data then...
    {
      Serial.readBytesUntil('\n', time_h2, 2);                  // read bytes and save it to buffer time_h2
      (*time_h) = (time_h2[0] - 0x30) * 10 + time_h2[1] - 0x30; // numbers from 0 to 9 in ASCII starts from 0x30
      Serial.println(*time_h);
      lcd.setCursor(4, 1);
      // if ((*time_h) < 10)
      // lcd.print("0");
      lcd.print(*time_h);
    }
  }

  Serial.print("Set minutes: ");
  uint8_t time_min2[2] = {0x00, 0x00};
  while ((time_min2[0] == 0x00) & (time_min2[1] == 0x00))
  {
    if (Serial.available() > 0)
    {
      Serial.readBytesUntil('\n', time_min2, 2);
      (*time_min) = (time_min2[0] - 0x30) * 10 + time_min2[1] - 0x30;
      Serial.println(*time_min);
      lcd.setCursor(7, 1);
      // if ((*time_min) < 10)
      // lcd.print("0");
      lcd.print(*time_min);
    }
  }
  state = 3;
}

void lcd_time_set_manual(uint8_t(*time_h), uint8_t(*time_min)) // time seting by poten and buttons
{
  bool button_check = 0;
  Serial.print("Set hour: ");
  while (!button_check) // wait until user press set_button
  {
    (*time_h) = analogRead(potentiometer) * 6 / 256; // 24/1024=6/256
    lcd.setCursor(4, 1);
    if ((*time_h) < 10)
      lcd.print("0");
    lcd.print(*time_h);
    if (!(PINB & (1 << PB1))) // check if set_button was pressed
      button_check = 1;
  }
  button_check = 0;
  Serial.print("Set minutes: ");
  waiting_loop();       // waiting loop which gives time to user for release set_button
  while (!button_check) // wait until user press set_button
  {
    (*time_min) = analogRead(potentiometer) * 15 / 256; // 60/1024=15/256
    lcd.setCursor(7, 1);
    if ((*time_min) < 10)
      lcd.print("0");
    lcd.print(*time_min);
    if (!(PINB & (1 << PB1))) // check if set_button was pressed
      button_check = 1;
  }
  state = 3; // go to next state
}

void waiting_loop() // Waiting loop
{
  for (long int i = 0; i < (F_CPU / 16); i++) // F_CPU -Arduino work frequancy
    asm("");
}

void setup()
{
  lcd.begin(16, 2); // Settings for LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Hello unknown! ");
  waiting_loop();

  Serial.begin(115200);

  DDRB = (0 << PB1) | (0 << PB2);  // Initialize PINs which we used from PORT C
  PORTB = (1 << PB1) | (1 << PB2); // Enable internal pullup resistor to INPUT PC0, PC1, PC2

  pinMode(potentiometer, INPUT);

  TCCR1A = 0x00;      // Reset timer1 control regA
  TCCR1B = 0x0D;      // Set CS bits to prescaler of 1024 and set WGM bits to CTC (mode 4)
  TCNT1 = 0x00;       // Reset timer1
  OCR1A = PRESCALER2; // Set output compare for interrupt period of 1sec
  TIMSK1 = 0x02;      // Enable output compare match interrupt A

  PCICR = (1 << 0); // Enable pin change interrupt on PIN2 of PORT B
  PCMSK0 = (1 << PB2);
  sei(); // Enable global interrupts

  /*for (int y=0; y<20; y++) //Checking the rounding performed by abs()
  {
    int x=abs(1+y/10);
    Serial.print(y+1);
    Serial.print(". ");
    Serial.println(x);
  }*/
}
void loop()
{
  if (state == 3)
  {
    lcd.setCursor(0, 0);
    lcd.print(" Measured time: ");
    lcd_print_time(&time_sec, &time_min, &time_h);
  }
  else if (state == 2)
  {
    lcd_time_set_manual(&time_h, &time_min);
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("    Set time:   ");
    lcd.setCursor(0, 3);
    lcd.print("    HH:MM:SS    ");
    waiting_loop();
    state = 2;
  }
}

ISR(TIMER1_COMPA_vect)
{
  one_sec = abs(TCNT1 / PRESCALER2); // Convert clock ticks to milisec 1 tick is 64us that if we have 15625 ticks it will be 1s
  if (one_sec == 1)
  {
    if (state == 3)
    {
      time_sec++;
      time_sec = (1 << 7) | time_sec; // 8th bit are used to do once change displaing value on LCD
    }
  }
}

ISR(PCINT0_vect)
{
  if (PINB & (1 << PB2))
    state = 1; // if user press Fun_Button go to state=1 which allow to set new time
}