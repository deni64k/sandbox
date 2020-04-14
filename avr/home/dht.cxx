#include <inttypes.h>
#include <math.h>
#include <stdlib.h>

#include <util/delay.h>

#include "common.hxx"
#include "dht.hxx"

void DHT::begin() {
  avr::pin_mode(*ddr_, *bus_, pin_, avr::kInputPullup);
  lastreadtime_ = 0;
}

float DHT::read_temperature(bool is_fahrenheit, bool force) {
  float result = NAN;

  if (!read(force)) {
    return result;
  }

  switch (type_) {
    case kDHT11:
      result = data_[2];
      if (data_[3] & 0x80) {
        result = -1.0f - result;
      }
      result += (data_[3] & 0x0f) * 0.1f;
      break;
    case kDHT12:
      result = data_[2];
      result += (data_[3] & 0x0f) * 0.1f;
      if (data_[2] & 0x80) {
        result *= -1.0f;
      }
      break;
    case kDHT21:
    case kDHT22:
      result = (std::uint16_t{(data_[2] & 0x7f)} << 8) | data_[3];
      result *= 0.1f;
      if (data_[2] & 0x80) {
        result *= -1.0f;
      }
      break;
  }

  if (is_fahrenheit) {
    result = C_to_F(result);
  }

  return result;
}

float DHT::read_humidity(bool force) {
  float result = NAN;

  if (!read(force)) {
    return result;
  }
    
  switch (type_) {
    case kDHT11:
    case kDHT12:
      result = data_[0] + data_[1] * 0.1f;
      break;
    case kDHT22:
    case kDHT21:
      result = (std::uint16_t{data_[0]} << 8) | data_[1];
      result *= 0.1f;
      break;
  }

  return result;
}

float DHT::compute_heat_index(bool is_fahrenheit) {
  return compute_heat_index(read_temperature(is_fahrenheit),
                            read_humidity(),
                            is_fahrenheit);
}

// Using both Rothfusz and Steadman's equations
// (http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml)
float DHT::compute_heat_index(float temperature,
                              float percentHumidity,
                              bool is_fahrenheit) noexcept {
  float hi;

  if (!is_fahrenheit)
    temperature = C_to_F(temperature);

  hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) +
              (percentHumidity * 0.094));

  if (hi > 79) {
    hi = -42.379 + 2.04901523 * temperature + 10.14333127 * percentHumidity +
         -0.22475541 * temperature * percentHumidity +
         -0.00683783 * pow(temperature, 2) +
         -0.05481717 * pow(percentHumidity, 2) +
         0.00122874  * pow(temperature, 2) * percentHumidity +
         0.00085282  * temperature * pow(percentHumidity, 2) +
         -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if ((percentHumidity < 13) &&
        (temperature >= 80.0) &&
        (temperature <= 112.0)) {
      hi -= ((13.0 - percentHumidity) * 0.25) *
            sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);
    } else if ((percentHumidity > 85.0) &&
               (temperature >= 80.0) &&
               (temperature <= 87.0)) {
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
    }
  }

  return is_fahrenheit ? hi : F_to_C(hi);
}

bool DHT::read(bool force) {
  millis_t now = millis_get();
  if (!force && now - lastreadtime_ < kMinInterval) {
    return last_result_;
  }

  lastreadtime_ = now;

  data_[0] = data_[1] = data_[2] = data_[3] = data_[4] = 0;

#if defined(ESP8266)
  yield();  // handle WiFi / reset software watchdog
#endif

  // Send start signal.  See DHT datasheet for full signal diagram:
  //   http://www.adafruit.com/datasheets/Digital%20humidity%20and%20temperature%20sensor%20AM2302.pdf

  // Go into high impedence state to let pull-up raise data line level and
  // start the reading process.
  avr::pin_mode(*ddr_, *bus_, pin_, avr::kInputPullup);
  _delay_ms(1);

  avr::pin_mode(*ddr_, *bus_, pin_, avr::kOutput);
  avr::digital_write(*bus_, pin_, avr::kLow);
  switch (type_) {
    case kDHT21:
    case kDHT22:
      _delay_us(1000); // data sheet says "at least 1ms"
      break;
    case kDHT11:
    default:
      _delay_ms(18); // data sheet says at least 18ms, 20ms just to be safe
      break;
  }

  // avr::digital_write(*bus_, pin_, avr::kHigh);
  // _delay_us(40); // data sheet says at least 18ms, 20ms just to be safe

  CyclesN cycles[80];
  {
    // End the start signal by setting data line high for 40 microseconds.
    avr::pin_mode(*ddr_, *bus_, pin_, avr::kInputPullup);
    // Delay a moment to let sensor pull data line low.
    _delay_us(pull_time_us_);

    // Now start reading the data line to get the value from the DHT sensor.

    // Turn off interrupts temporarily because the next sections
    // are timing critical and we don't want any interruptions.
    avr::InterruptLock lock;

    // First expect a low signal for ~80 microseconds followed by a high signal
    // for ~80 microseconds again.
    if (expect_pulse(avr::kLow) == kTimeout) {
      // DHT timeout waiting for start signal low pulse.
      last_result_ = false;
      return last_result_;
    }

    if (expect_pulse(avr::kHigh) == kTimeout) {
      // DHT timeout waiting for start signal high pulse.
      last_result_ = false;
      return last_result_;
    }

    // Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
    // microsecond low pulse followed by a variable length high pulse.  If the
    // high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
    // then it's a 1.  We measure the cycle count of the initial 50us low pulse
    // and use that to compare to the cycle count of the high pulse to determine
    // if the bit is a 0 (high state cycle count < low state cycle count), or a
    // 1 (high state cycle count > low state cycle count). Note that for speed
    // all the pulses are read into a array and then examined in a later step.
    for (int i = 0; i < 80; i += 2) {
      cycles[i]     = expect_pulse(avr::kLow);
      cycles[i + 1] = expect_pulse(avr::kHigh);
    }
  } // Timing critical code is now complete.

    // Inspect pulses and determine which ones are 0 (high state cycle count < low
    // state cycle count), or 1 (high state cycle count > low state cycle count).
  for (int i = 0; i < 40; ++i) {
    std::uint32_t low_cycles  = cycles[2 * i];
    std::uint32_t high_cycles = cycles[2 * i + 1];
    if ((low_cycles == kTimeout) || (high_cycles == kTimeout)) {
      // DHT timeout waiting for pulse.
      last_result_ = false;
      return last_result_;
    }
    data_[i / 8] <<= 1;
    // Now compare the low and high cycle times to see if the bit is a 0 or 1.
    if (high_cycles > low_cycles) {
      // High cycles are greater than 50us low cycle count, must be a 1.
      data_[i / 8] |= 1;
    }
    // Else high cycles are less than (or equal to, a weird case) the 50us low
    // cycle count so this must be a zero.  Nothing needs to be changed in the
    // stored data.
  }

  // Check we read 40 bits and that the checksum matches.
  if (data_[4] == ((data_[0] + data_[1] + data_[2] + data_[3]) & 0xff)) {
    last_result_ = true;
  } else {
    // DHT checksum failure!
    last_result_ = false;
  }

  return last_result_;
}

DHT::CyclesN DHT::expect_pulse(avr::PinLevel level) {
  CyclesN count = 0;

  std::uint8_t const bit = 1 << pin_;
  std::uint8_t const port_state = (level == avr::kHigh) ? bit : 0;
  while ((*avr::port_input_register(*bus_) & bit) == port_state) {
    if (count >= maxcycles_) {
      return kTimeout;
    }
    ++count;
  }

  return count;
}
  
