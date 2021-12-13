#include <Arduino.h>
#include <avr/power.h>

#define LED_PWR PB0
#define LED_RED PB3
#define LED_GREEN PB4
#define VOLT_IN PB2
/*                                  ____
          RST A0  D5 PB5          1|o   |8  Vcc
      A3  D3 PB3 RED_GND  --|--<| 2|    |7  PB2 A1   --|--<| VOLT_IN
      A2  D4 PB4 GRN_GND  --|--<| 3|    |6  PB1 D1   --|>--| N/C
                     GND          4|____|5  PB0 D0   --|>--| LED_PWR
*/

#define NUMSAMP 32
uint8_t    ndx = 0;
uint16_t   tot = 0;
uint16_t   dat[NUMSAMP] = {0}; //avg over NUMSAMP readings

enum State
{
  SS_Normal,      // no light
  SS_OverCharge,  // green red alternate
  SS_UnderCharge, // Amber steady
  SS_NoCharge1,   // red 1 flash rate
  SS_NoCharge2,   // red 2 flash rate
  SS_NoCharge3,   // red 3 flash rate
  SS_NoCharge4,   // red 4 flash rate
};

enum Color { Off, Red, Green, Amber };

// The divider is 39k/12k giving a 4.25x divider for VIN
// thus 2.5v input on ADC pin is 4.25 x 2.5v = 10.625v input = 512 ADC
// note: maximum value on the divider is 21.25v
// the factor is determined as VCC / 1023 ADC steps * 4.25
//   5.0v / 1023 * FACTOR * 1000 = Actual MilliVolts per ADC step
//RE - trimmed for 38.9K/12.1K due to tolerance error, ideally this trim would be written into EEPROM 
#define V_CONV_FACTOR 20.3295

/*
  Set the LED color to a defined value, PWR pin is used to drive the brightness from PWM
*/
void led(Color color)
{
  analogWrite(LED_PWR, 0);
  switch (color)
  {
    case Off:
      digitalWrite(LED_RED, 1);
      digitalWrite(LED_GREEN, 1);
      analogWrite(LED_PWR, 0);
    break;
    case Red:
      digitalWrite(LED_RED, 0);
      digitalWrite(LED_GREEN, 1);
      analogWrite(LED_PWR, 25);
    break;
    case Green:
      digitalWrite(LED_RED, 1);
      digitalWrite(LED_GREEN, 0);
      analogWrite(LED_PWR, 15);
    break;
    case Amber:
      digitalWrite(LED_RED, 0);
      digitalWrite(LED_GREEN, 0);
      analogWrite(LED_PWR, 25);
    break;
  }
}

/*
  Flashes the led a number of times, then pause
*/
void flashLed(uint8_t flashes, Color color)
{
  for (uint8_t i = 0; i < flashes; i++)
  {
    led(color);
    delay(300);
    led(Off);
    delay(200);
  }
  delay(1250 * flashes);
}

/*
  ISR to add latest reading to our rolling average
*/
ISR(ADC_vect)
{
  int w = ADC;
  tot -= dat[ndx]; //subtract old value from tot
  tot += w;        //add in new value
  dat[ndx++] = w;  //remember new value, bump ndx
  if (ndx == NUMSAMP)
    ndx = 0; //rewind

  /*
	Set the ADC Start Conversion bit on the ADC Control and Status Register A
	*/
  ADCSRA |= (1 << ADSC); // start a conversion
}

/*
  convert the average ADC into a millivolt value
  determine the visual state to display as a result of the voltage
*/
State determineNextState()
{
  uint16_t mVolts = tot / NUMSAMP * V_CONV_FACTOR;
  if (mVolts > 15200)
  {
    return SS_OverCharge;
  }
  if (mVolts > 13200)
  {
    return SS_Normal;
  }
  if (mVolts > 12450)
  {
    return SS_UnderCharge;
  }
  if (mVolts > 12250)
  {
    return SS_NoCharge1;
  }
  if (mVolts > 12050)
  {
    return SS_NoCharge2;
  }
  if (mVolts > 11800)
  {
    return SS_NoCharge3;
  }
  return SS_NoCharge4;
}

/*
Loop is called infinitely
We perform the state machine logic here 
*/
void loop()
{
  //determine the state to switch to
  State currentState = determineNextState();
  switch (currentState)
  {
  case SS_OverCharge:
    led(Amber);
    break;
  case SS_Normal:
    led(Off);
    break;
  case SS_UnderCharge:
    led(Red);
    break;
  case SS_NoCharge1:
    flashLed(1,Red);
    break;
  case SS_NoCharge2:
    flashLed(2,Red);
    break;
  case SS_NoCharge3:
    flashLed(3,Red);
    break;
  case SS_NoCharge4:
    flashLed(4,Red);
    break;
  }
  // delay so that state changes are not frantic for the user
  delay(2500);
}

/*
Setup is called once when the board boots.
*/
void setup()
{
  clock_prescale_set(clock_div_16);

  // init ADC input for the pin that reads the input voltage
  pinMode(VOLT_IN, INPUT);

  // initialize LED digital pins as output for system status
  pinMode(LED_PWR, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Setup the ADC and attach to interrupt 
  ADMUX = (1 << MUX0); // use VCC for AREF, use ADC1 for input (PA1), MUX 5:0 bits 00001
  ADCSRA =
      (1 << ADEN) |                               // Enable ADC
      (0 << ADSC) |                               // start ADC
      (0 << ADATE) |                              // disable auto triggering of ADC
      (0 << ADIF) |                               // clear ADC interrupt flag
      (1 << ADIE) |                               // Enable ADC interrupt
      (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // set prescaler to /128 1:1:1
  ADCSRA |= (1 << ADSC);                          // start a conversion

  // Start up sequence
  flashLed(1, Green);
}
