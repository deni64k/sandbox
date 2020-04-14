#pragma once

#include <inttypes.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>

namespace std {

using ::uint8_t;
using ::uint16_t;
using ::uint32_t;
using ::int8_t;
using ::int16_t;
using ::int32_t;
using byte = ::uint8_t;
using ::size_t;

}  // namespace std

constexpr std::uint8_t bit(std::uint8_t bits, std::uint8_t pos) noexcept {
  return bits & (0x01 << pos);
}

template <typename T> constexpr T us_to_clock_cycles(T us) noexcept {
  return us * (F_CPU / 1000000L);
}

namespace avr {

enum PinMode {
  kOutput      = 0,
  kInput       = 1,
  kInputPullup = 2
};

enum PinLevel {
  kLow  = 0,
  kHigh = 1
};

namespace {

inline void
pin_mode(volatile std::uint8_t& ddr, volatile std::uint8_t& bus, std::uint8_t pin, PinMode mode) {
  volatile std::uint8_t* reg = &ddr;
  volatile std::uint8_t* out = &bus;

  std::uint8_t const bit = 1 << pin;

  std::uint8_t oldSREG = SREG;
  cli();

  switch (mode) {
    case kInput:
      *reg &= ~bit;
      *out &= ~bit;
      break;
    case kInputPullup:
      *reg &= ~bit;
      *out |= bit;
      break;
    case kOutput:
      *reg |= bit;
      break;
  }

  SREG = oldSREG;
}

inline void
digital_write(volatile std::uint8_t& bus, std::uint8_t pin, PinLevel level) {
  std::uint8_t oldSREG = SREG;
  cli();

  switch (level) {
    case kHigh:
      bus |= 1 << pin;
      break;
    case kLow:
      bus &= ~(1 << pin);
      break;
  }

  SREG = oldSREG;
}

inline PinLevel
digital_read(volatile std::uint8_t& bus, std::uint8_t pin) {
  return bus & (1 << pin) ? kHigh : kLow;
}

inline volatile std::uint8_t*
port_input_register(volatile std::uint8_t& bus) {
  if (&bus == &PORTB)
    return &PINB;
  if (&bus == &PORTC)
    return &PINC;
  if (&bus == &PORTD)
    return &PIND;
}

}  // namespace

struct InterruptLock {
  InterruptLock() {
    cli();
  }
  InterruptLock(InterruptLock const&) = delete;
  InterruptLock(InterruptLock&&) = delete;

  ~InterruptLock() {
    sei();
  }
};

}  // namespace avr
