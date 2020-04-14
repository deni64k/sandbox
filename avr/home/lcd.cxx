#pragma once

#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>

#include "common.hxx"
#include "lcd.hxx"

using namespace avr;

// Based on
// * https://github.com/arduino-libraries/LiquidCrystal/blob/master/src/LiquidCrystal.cpp

LCD::LCD(volatile std::uint8_t& ctl_ddr,
         volatile std::uint8_t& ctl_bus,
         unsigned int const pinEN,
         unsigned int const pinRW,
         unsigned int const pinRS,
         volatile std::uint8_t& data_ddr,
         volatile std::uint8_t& data_bus,
         unsigned int const pinD0,
         unsigned int const pinD1,
         unsigned int const pinD2,
         unsigned int const pinD3):
ctl_ddr_{&ctl_ddr},
ctl_bus_{&ctl_bus},
pin_en_{pinEN},
pin_rw_{pinRW},
pin_rs_{pinRS},
data_ddr_{&data_ddr},
data_bus_{&data_bus},
pin_data_({pinD0, pinD1, pinD2, pinD3}) {
  display_function_ = k4BitMode | k2Line | k5x8Dots;
}

void LCD::init() {  
  pin_mode(*ctl_ddr_, *ctl_bus_, pin_en_, avr::kOutput);
  pin_mode(*ctl_ddr_, *ctl_bus_, pin_rw_, avr::kOutput);
  pin_mode(*ctl_ddr_, *ctl_bus_, pin_rs_, avr::kOutput);
  for (int i = 0; i < ((display_function_ | k4BitMode) ? 4 : 8); ++i) {
    pin_mode(*data_ddr_, *data_bus_, pin_data_[i], avr::kOutput);
  }

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
  _delay_ms(50);

  digital_write(*ctl_bus_, pin_en_, avr::kLow);
  digital_write(*ctl_bus_, pin_rw_, avr::kLow);
  digital_write(*ctl_bus_, pin_rs_, avr::kLow);

  if (display_function_ | k4BitMode) {
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46
    write_4bits(0x03);
    _delay_us(4500);
    write_4bits(0x03);
    _delay_us(4500);
    write_4bits(0x03);
    _delay_us(150);
    write_4bits(0x02);
  }

  write_command(kFunctionSet | display_function_);

  display_control_ = kDisplayOn | kCursorOff | kBlinkOff;
  display();

  clear();

  display_mode_ = kEntryLeft | kEntryShiftDecrement;
  write_command(kEntryModeSet | display_mode_);
}

void LCD::clear() {
  write_command(kClearDisplay);
  _delay_us(2000);
}

void LCD::home() {
  write_command(kReturnHome);
  _delay_us(2000);
}

void LCD::display_off() {
  display_control_ &= ~kDisplayOn;
  write_command(kDisplayControl | display_control_);
}

void LCD::display() {
  display_control_ |= kDisplayOn;
  write_command(kDisplayControl | display_control_);
}

void LCD::cursor_off() {
  display_control_ &= ~kCursorOn;
  write_command(kDisplayControl | display_control_);
}

void LCD::cursor() {
  display_control_ |= kCursorOn;
  write_command(kDisplayControl | display_control_);
}

void LCD::blink_off() {
  display_control_ &= ~kBlinkOn;
  write_command(kDisplayControl | display_control_);
}

void LCD::blink() {
  display_control_ |= kBlinkOn;
  write_command(kDisplayControl | display_control_);
}

// These commands scroll the display without changing the RAM.
void LCD::scroll_display_left() {
  write_command(kCursorShift | kDisplayMove | kMoveLeft);
}

void LCD::scroll_display_right() {
  write_command(kCursorShift | kDisplayMove | kMoveRight);
}

// This is for text that flows Left to Right.
void LCD::left_to_right() {
  display_mode_ |= kEntryLeft;
  write_command(kEntryModeSet | display_mode_);
}

// This is for text that flows Right to Left.
void LCD::right_to_left() {
  display_mode_ &= ~kEntryLeft;
  write_command(kEntryModeSet | display_mode_);
}

// This will 'right justify' text from the cursor.
void LCD::autoscroll() {
  display_mode_ |= kEntryShiftIncrement;
  write_command(kEntryModeSet | display_mode_);
}

// This will 'left justify' text from the cursor.
void LCD::autoscroll_off(void) {
  display_mode_ &= ~kEntryShiftIncrement;
  write_command(kEntryModeSet | display_mode_);
}

void LCD::set_cursor(uint8_t row, uint8_t col) {
  // const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
  // if ( row >= max_lines ) {
  //   row = max_lines - 1;    // we count rows starting w/0
  // }
  // if ( row >= _numlines ) {
  //   row = _numlines - 1;    // we count rows starting w/0
  // }
  
  // command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));

  write_command(kSetDDRAMAddr | (col + 0x40 * row));
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters.
void LCD::create_char(std::uint8_t location, std::uint8_t charmap[8]) {
  location &= 0x07; // we only have 8 locations 0-7
  write_command(kSetCGRAMAddr | (location << 3));
  for (int i = 0; i < 8; ++i) {
    write_char(charmap[i]);
  }
}

void LCD::write(char ch) {
  write_char(ch);
}

void LCD::write(char const* ch) {
  std::size_t const maxlen = 20;

  for (int i = 0; *ch && i < maxlen; ++i, ++ch) {
    write_char(*ch);
  }
}

void LCD::write(float f, int digits) {
  if (isnan(f)) {
    write("NAN");
    return;
  }

  char s[20];
  char* ch = &s[19];

  float basis = 1;
  for (int i = 0; i < digits; ++i) {
    basis *= 0.1;
  }

  for (int i = 19; i >= 0; --i, --ch) {
    if (f == 0.0f) {
      break;
    }

    if (basis == 1 && i != 19) {
      *ch = '.';
      basis *= 10.0f;
      continue;
    }

    std::uint8_t d = static_cast<unsigned int>(f / basis) % 10;
    *ch = '0' + d;
    f -= f * basis;
    basis *= 10.0f;
  }
    
  write(ch);
}

void LCD::write(unsigned int x) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%u", x);
  write(buf);
}

void LCD::write_command(std::uint8_t value) {
  digital_write(*ctl_bus_, pin_rs_, avr::kLow);
  digital_write(*ctl_bus_, pin_rw_, avr::kLow);

  if (display_function_ | k4BitMode) {
    write_4bits(value >> 4);
    write_4bits(value);
  } else {
    write_8bits(value);
  }
}

void LCD::write_char(std::uint8_t value) {
  digital_write(*ctl_bus_, pin_rs_, avr::kHigh);
  digital_write(*ctl_bus_, pin_rw_, avr::kLow);

  if (display_function_ | k4BitMode) {
    write_4bits(value >> 4);
    write_4bits(value);
  } else {
    write_8bits(value);
  }
}

void LCD::write_4bits(std::uint8_t bits) {
  for (std::size_t i = 0; i < 4; ++i) {
    avr::digital_write(*data_bus_, pin_data_[i], bit(bits, i) ? avr::kHigh : avr::kLow);
  }

  pulse_enable();
}

void LCD::write_8bits(std::uint8_t bits) {
  for (std::size_t i = 0; i < 8; ++i) {
    avr::digital_write(*data_bus_, pin_data_[i], bit(bits, i) ? avr::kHigh : avr::kLow);
  }

  pulse_enable();
}

void LCD::pulse_enable() {
  digital_write(*ctl_bus_, pin_en_, avr::kLow);
  _delay_us(1);

  digital_write(*ctl_bus_, pin_en_, avr::kHigh);
  _delay_us(1);

  digital_write(*ctl_bus_, pin_en_, avr::kLow);
  _delay_us(100);
}
