#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned char hours_1 = 0, hours_0 = 0, minutes_1 = 0, minutes_0 = 0, secound_0 = 0, secound_1 = 0, treadmill_running = 1;
int speed_motor = 128;


void Timer1_CTC(void)
{
  /*
     1- choose prescalar from CS10, CS11, CS12
     2- Choose mode of operation from table
     3- write the initial value you want to start in TCNT1
     4- Put the value you want to compare
     5- enable Interrupt
     6- if not a pwm mode enable FOC
     7- Enable global interrupt
  */
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
  TCNT1 = 0;
  OCR1A = 1000;
  TIMSK |= (1 << OCIE1A);
  TCCR1A = (1 << FOC1A);
}
void PWM_Timer0_Init(unsigned char set_duty_cycle)
{

  TCNT0 = 0; //Set Timer Initial value

  OCR0  = set_duty_cycle; // Set Compare Value

  DDRB  = DDRB | (1 << PB3); //set PB3/OC0 as output pin --> pin where the PWM signal is generated from MC.

  /* Configure timer control register
     1. Fast PWM mode FOC0=0
     2. Fast PWM Mode WGM01=1 & WGM00=1
     3. Clear OC0 when match occurs (non inverted mode) COM00=0 & COM01=1
     4. clock = F_CPU/8 CS00=0 CS01=1 CS02=0
  */
  TCCR0 = (1 << WGM00) | (1 << WGM01) | (1 << COM01) | (1 << CS01);
}

void INT1_Init(void)
{
  DDRD &= ~(1 << PD3);

  // Enable INT1
  GICR |= (1 << INT1);

  // Set INT1 to triger on falling edge (or rising edge based on your requirement)
  MCUCR |= (1 << ISC11);
  MCUCR &= ~(1 << ISC10);
}

void init_external_interrupts() {
  // Configure INT0 for rising edge detection
  MCUCR |= (1 << ISC01) | (1 << ISC00); // ISC01 = 1, ISC00 = 1 for rising edge

  // Configure INT2 for falling edge detection
  MCUCSR &= ~(1 << ISC2); // ISC2 = 0 for falling edge

  // Enable INT0 and INT2 interrupts
  GICR |= (1 << INT0) | (1 << INT2);

}

ISR(INT0_vect) {
  // Increase motor speed on INT0 interrupt (rising edge)
  if (speed_motor <= 235) {
    speed_motor += 20; // Increase speed by 10 (adjust as needed)

  } PWM_Timer0_Init(speed_motor);
}

ISR(INT1_vect)
{
  // Toggle treadmill running state
  treadmill_running ^=1;

  if (treadmill_running)
  { speed_motor = 128;
    PWM_Timer0_Init(speed_motor);
    PORTB |= (1 << 0);
    PORTB &= ~(1 << 1);

  }
  else
  {

    // Stop treadmill by disabling Timer0 PWM
    speed_motor = 0;
    PWM_Timer0_Init(speed_motor);
    hours_1 = 0, hours_0 = 0, minutes_1 = 0, minutes_0 = 0, secound_0 = 0, secound_1 = 0;
    PORTA = 0;

    PORTB &= ~(1 << 0) & (1 << 1);
  }
}
ISR(INT2_vect) {
  // Decrease motor speed on INT2 interrupt (falling edge)
  if (speed_motor >=20) {
    speed_motor -= 20; // Decrease speed by 10 (adjust as needed)
  }
  PWM_Timer0_Init(speed_motor);

}
ISR(TIMER1_COMPA_vect)
{
  // Check if overflow occurs at the 7-segment as its maximum count is 9
  if (secound_0 == 9)
  {
    secound_0 = 0;

    if (secound_1 == 7)
    {
      secound_1 = 0;
      if (minutes_0 == 9)
      {
        minutes_0 = 0;
        if (minutes_1 == 7)
        {
          minutes_1 = 0;
          if (hours_0 == 9)
          {
            hours_0 = 0;
            if (hours_1 == 9)
            {
              hours_1 = 0, hours_0 = 0, minutes_1 = 0, minutes_0 = 0, secound_0 = 0, secound_1 = 0;
            }
            else
              hours_1++;
          }
          else
            hours_0++;
        }
        else
          minutes_1++;
      }
      else
        minutes_0++;
    }
    else
      secound_1++;
  }
  else
    secound_0++;
}
int main()
{
  DDRB |= (1 << 0) | (1 << 1); // motor input voltage
  DDRC |= 0x0F;    // Configure the first four pins in PORTC as output pins.
  PORTC &= 0xF0;   // Initialize the 7-seg display to zero at the beginning.
  DDRB &= ~((1 << 4) | (1 << 5));  // Configure PB4 and PB5 as input pins.
  DDRD &= (~(1 << 3)); //Set pin pd3 as input from pir sensor
  SREG |= (1 << 7);  // Enable global interrupts in MC.
  PORTB &= ~(1 << 0) & (1 << 1);
  init_external_interrupts();
  Timer1_CTC();  // Start the timer.
  INT1_Init();   // Initialize INT1
  PWM_Timer0_Init(speed_motor);  // Generate PWM with duty cycle
  unsigned char flag = 0;
  while (1)
  {

    if (treadmill_running) {

      if ((PINB & (1 << 5)))  // Check if the button connected to PB4 is pressed
      {
        TCCR1B = 0 ; // Stop the timer


        PWM_Timer0_Init(0);
      }

      else  if ((PINB & (1 << 4)) || flag) // Check if the button connected to PB5 is pressed
      { TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);  // Resume the timer
        flag = 0;
        speed_motor=128;
        PWM_Timer0_Init(speed_motor);



      }
      PORTA = (1 << 0);
      PORTC = (PORTC & 0xf0) | (hours_1 & 0x0F);
      _delay_ms(2);

      PORTA = (1 << 1);
      PORTC = (PORTC & 0xf0) | (hours_0 & 0x0F);
      _delay_ms(2);

      PORTA = (1 << 2);
      PORTC = (PORTC & 0xf0) | (minutes_1 & 0x0F);
      _delay_ms(2);

      PORTA = (1 << 3);
      PORTC = (PORTC & 0xf0) | (minutes_0 & 0x0F);
      _delay_ms(2);

      PORTA = (1 << 4);
      PORTC = (PORTC & 0xf0) | (secound_1 & 0x0F);
      _delay_ms(2);

      PORTA = (1 << 5);
      PORTC = (PORTC & 0xf0) | (secound_0 & 0x0F);
      _delay_ms(2);


    }
    else
    { PORTA = 0;
      TCCR1B = 0 ; // Stop the timer
      flag = 1;
    }

  }
}
