#include <avr/power.h>
#include <util/atomic.h>

#include "common.hxx"
#include "millis.hxx"

static volatile millis_t millis_ = 0;

void millis_init() {
  avr::InterruptLock lock();

  // Set timer2 interrupt at 1kHz.
  TCCR2A = 0;  // set entire TCCR2A register to 0
  TCCR2B = 0;  // same for TCCR2B
  TCNT2  = 0;  // initialize counter value to 0
  // Set compare match register for 1khz increments.
  // OCR2A = 124;  // 20e6 / 64 / 2500 - 1 = F_CPU / (2500*64) - 1 // (must be <256)
  // OCR2A = 99;  // 16e6 / 64 / 2500 - 1 = F_CPU / (2500*64) - 1 // (must be <256)
  OCR2A = 250;  // F_CPU / 64ULL / 1000ULL; // (must be <256)
  // Turn on CTC mode.
  TCCR2A |= _BV(WGM21);
  // Set CS22 bit for 64 prescaler.
  TCCR2B |= _BV(CS22);
  // Enable timer compare interrupt.
  TIMSK2 |= _BV(OCIE2A);
}

millis_t millis_get() {
  millis_t ms;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ms = millis_;
  }

  return ms;
}

ISR(TIMER2_COMPA_vect) {
  ++millis_;
}
