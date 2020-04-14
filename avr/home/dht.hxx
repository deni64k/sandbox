#pragma once

#include "common.hxx"
#include "millis.hxx"

struct DHT {
  static std::uint32_t const kTimeout = -1;

  enum DHDTType {
    kDHT11  = 11,
    kDHT12  = 12,
    kDHT21  = 21,
    kDHT22  = 22,
    kAM2301 = 21
  };

  static constexpr millis_t kMinInterval = 2000;

  static constexpr float C_to_F(float c) noexcept { return c * 1.8 + 32; }
  static constexpr float F_to_C(float f) noexcept { return (f - 32) * 0.55555; }

  DHT(volatile std::uint8_t& ddr,
      volatile std::uint8_t& bus,
      unsigned int const pin):
  ddr_{&ddr},
  bus_{&bus},
  pin_{pin},
  type_{kDHT11}
  {}

  void begin();

  float read_temperature(bool is_fahrenheit = true, bool force = false);

  float read_humidity(bool force = false);
  float compute_heat_index(bool is_fahrenheit = true);

  // Using both Rothfusz and Steadman's equations
  // (http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml)
  static float compute_heat_index(float temperature,
                                  float percentHumidity,
                                  bool is_fahrenheit = true) noexcept;

 protected:
#if F_CPU > 16000000L
  using CyclesN = std::uint32_t;
#else
  using CyclesN = std::uint16_t;
#endif

  bool read(bool force = false);

  CyclesN expect_pulse(avr::PinLevel level);
  
 public:
  static constexpr CyclesN const maxcycles_ = us_to_clock_cycles(1000);

  volatile std::uint8_t* const ddr_;
  volatile std::uint8_t* const bus_;
  unsigned int const pin_;

  DHDTType const type_;

  static constexpr unsigned int const pull_time_us_ = 40;

  std::uint8_t data_[5];
  bool last_result_;
  millis_t lastreadtime_;
};
