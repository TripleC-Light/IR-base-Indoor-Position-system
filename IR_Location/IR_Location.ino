#include <IRremote.h>

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define PB1 4

#define IRLocate_BITS          8
#define IRLocate_HDR_MARK    1000
#define IRLocate_HDR_SPACE   1000
#define IRLocate_BIT_MARK     300
#define IRLocate_ONE_SPACE    600
#define IRLocate_ZERO_SPACE   300

IRsend irsend; // IRRemote限定使用數位腳位3

unsigned long ONOFF = 0x75013101;
unsigned long turnZero = 0x75013102;
unsigned char snedID = 0;

int FSMstate = 0;
unsigned long But_t = 0, But_temp = 0;
volatile int f_wdt = 1;
long randNumber;

void setup()
{
  DDRB = 0x00;                                        // set PortB as input with internal pull-ups on
  PORTB = 0xFF;
  DDRD = 0x3F;                                        // set PortD6.D7 as input with internal pull-ups on
  PORTD = 0xC0;
  pinMode(PB1, INPUT_PULLUP);

  /*** Setup the WDT ***/
  /* Clear the reset flag. */
  MCUSR &= ~(1 << WDRF);
  /* In order to change WDE or the prescaler, we need to
     set WDCE (This will allow updates for 4 clock cycles).
  */
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  /* set new watchdog timeout prescaler value */
  //WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  WDTCSR = 1 << WDP0 | 1 << WDP2; /* 0.5 seconds */
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);

  randomSeed(analogRead(A0));

  // 關閉ADC，以免進入睡眠模式時依然在動作
  ADMUX &= 0x3F;
  ADCSRA &= 0x7F;
}

void loop() {
  if (f_wdt == 1)
  {
    /* Transfer IR Data */
    snedID = PINB;
    snedID = (snedID << 2) | ((PIND >> 6) & 0x03);
    if ( snedID == 0xFF ) {
      int i = 0;
      while ( snedID == 0xFF )
      {
        snedID = PINB;
        snedID = (snedID << 2) | ((PIND >> 6) & 0x03);
        IRLocateSend(i, IRLocate_BITS);
        randNumber = random(1, 5);
        //delay(500+(50*randNumber));
        delay(500 + (50 * randNumber));
        i++;
      }
    } else {
      randNumber = random(1, 10);
      delay(25 * randNumber);
      IRLocateSend(snedID, IRLocate_BITS);
      /* Don't forget to clear the flag. */
      f_wdt = 0;
      /* Re-enter sleep mode. */
      enterSleep();
    }
  }
  else
  {
    /* Do nothing. */
  }
}

void IRLocateSend(unsigned long data,  int nbits)
{
  // Set IR carrier frequency
  irsend.enableIROut(38);

  // Header
  irsend.mark(IRLocate_HDR_MARK);
  irsend.space(IRLocate_HDR_SPACE);

  // Data
  for (unsigned long  mask = 1UL << (nbits - 1);  mask;  mask >>= 1) {
    if (data & mask) {
      irsend.mark(IRLocate_BIT_MARK);
      irsend.space(IRLocate_ONE_SPACE);
    } else {
      irsend.mark(IRLocate_BIT_MARK);
      irsend.space(IRLocate_ZERO_SPACE);
    }
  }

  // Footer
  irsend.mark(IRLocate_BIT_MARK);
  irsend.space(0);  // Always end with the LED off
}

/***************************************************
    Name:        ISR(WDT_vect)

    Returns:     Nothing.

    Parameters:  None.

    Description: Watchdog Interrupt Service. This
                 is executed when watchdog timed out.

 ***************************************************/
ISR(WDT_vect)
{
  if (f_wdt == 0)
  {
    f_wdt = 1;
  }
  else
  {
    Serial.println("WDT Overrun!!!");
  }
}

/***************************************************
    Name:        enterSleep

    Returns:     Nothing.

    Parameters:  None.

    Description: Enters the arduino into sleep mode.

 ***************************************************/
void enterSleep(void)
{
  //set_sleep_mode(SLEEP_MODE_PWR_SAVE);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_SAVE for lowest power consumption. */
  sleep_enable();

  /* Now enter sleep mode. */
  sleep_mode();

  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */

  /* Re-enable the peripherals. */
  power_all_enable();
}
