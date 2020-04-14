#pragma once

#include "common.hxx"

// Based on
// * https://github.com/arduino-libraries/LiquidCrystal/blob/master/src/LiquidCrystal.cpp

static inline std::byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

static inline std::byte smiley[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000
};

struct LCD {
  static std::uint8_t const kClearDisplay   = 0x01;
  static std::uint8_t const kReturnHome     = 0x02;
  static std::uint8_t const kEntryModeSet   = 0x04;
  static std::uint8_t const kDisplayControl = 0x08;
  static std::uint8_t const kCursorShift    = 0x10;
  static std::uint8_t const kFunctionSet    = 0x20;
  static std::uint8_t const kSetCGRAMAddr   = 0x40;
  static std::uint8_t const kSetDDRAMAddr   = 0x80;

  // Flags for display entry mode.
  static std::uint8_t const kEntryRight          = 0x00;
  static std::uint8_t const kEntryLeft           = 0x02;
  static std::uint8_t const kEntryShiftIncrement = 0x01;
  static std::uint8_t const kEntryShiftDecrement = 0x00;

  // Flags for display on/off control.
  static std::uint8_t const kDisplayOn  = 0x04;
  static std::uint8_t const kDisplayOff = 0x00;
  static std::uint8_t const kCursorOn   = 0x02;
  static std::uint8_t const kCursorOff  = 0x00;
  static std::uint8_t const kBlinkOn    = 0x01;
  static std::uint8_t const kBlinkOff   = 0x00;

  // Flags for display/cursor shift.
  static std::uint8_t const kDisplayMove = 0x08;
  static std::uint8_t const kCursorMove  = 0x00;
  static std::uint8_t const kMoveRight   = 0x04;
  static std::uint8_t const kMoveLeft    = 0x00;

  static std::uint8_t const k4BitMode = 0x00;
  static std::uint8_t const k8BitMode = 0x10;
  static std::uint8_t const k1Line    = 0x00;
  static std::uint8_t const k2Line    = 0x08;
  static std::uint8_t const k5x8Dots  = 0x00;
  static std::uint8_t const k5x10Dots = 0x04;

  LCD(volatile std::uint8_t& ctl_ddr,
      volatile std::uint8_t& ctl_bus,
      unsigned int const pinEN,
      unsigned int const pinRW,
      unsigned int const pinRS,
      volatile std::uint8_t& data_ddr,
      volatile std::uint8_t& data_bus,
      unsigned int const pinD0,
      unsigned int const pinD1,
      unsigned int const pinD2,
      unsigned int const pinD3);

  void init();

  void clear();
  void home();

  void display_off();
  void display();

  void cursor_off();
  void cursor();

  void blink_off();
  void blink();

  // These commands scroll the display without changing the RAM.
  void scroll_display_left();
  void scroll_display_right();

  // This is for text that flows Left to Right.
  void left_to_right();
  // This is for text that flows Right to Left.
  void right_to_left();

  // This will 'right justify' text from the cursor.
  void autoscroll();
  // This will 'left justify' text from the cursor.
  void autoscroll_off();

  void set_cursor(uint8_t row, uint8_t col);

  // Allows us to fill the first 8 CGRAM locations
  // with custom characters.
  void create_char(std::uint8_t location, std::uint8_t charmap[8]);

  void write(char ch);
  void write(char const* ch);
  void write(float f, int digits = 2);
  void write(unsigned int);

 protected:
  void write_command(std::uint8_t value);
  void write_char(std::uint8_t value);
  void write_4bits(std::uint8_t bits);
  void write_8bits(std::uint8_t bits);
  void pulse_enable();

 private:
  volatile std::uint8_t* const ctl_ddr_;
  volatile std::uint8_t* const ctl_bus_;
  unsigned int const pin_en_;
  unsigned int const pin_rw_;
  unsigned int const pin_rs_;

  volatile std::uint8_t* const data_ddr_;
  volatile std::uint8_t* const data_bus_;
  unsigned int const pin_data_[8];

  unsigned int display_function_;
  unsigned int display_control_;
  unsigned int display_mode_;
};
