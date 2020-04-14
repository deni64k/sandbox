#include <avr/io.h>
#include <util/delay.h>

int main() {
  /*
  DDRD = 0xFF;         //PD as output
  PORTD = 0xff;        //keep all LEDs off

  while (1) {
    PORTD = 0b00000000;       //turn LED on
    _delay_ms(1000);          //wait for half second
    PORTD = 0b11111111;       //turn LED off
    _delay_ms(1000);          //wait for half second
  }
  */
  
  DDRB |= (1 << PB1); // set PD6 (pin 11) output

  for (;;) {
    PORTB |= (1 << PB1);
    _delay_ms(1000);
    PORTB &= ~(1 << PB1);
    _delay_ms(4000);
  }

  /*
  DDRB |= (1 << DDB1);
  for (;;) {
    PORTB |= (1 << PORTB1);
    _delay_ms(1000);
    PORTB &= ~ (1 << PORTB1);
    _delay_ms(1000);
  }
  */
  /*
    DDRD = 0b00000100; // Port D2 (Pin 4 in the ATmega) made output
    PORTD = 0b00000000; // Turn LED off

    while (1) {
    PORTD = 0b00000100; //Turn LED on
    _delay_ms(200); // delay of 200 millisecond
    PORTD = 0b00000000; //Turn LED off
    _delay_ms(200); // delay of 200 millisecond
    }
  */

  return 0;
}
