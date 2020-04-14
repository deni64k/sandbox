#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <avr/io.h>
#include <util/delay.h>

#include "lcd.hxx"
#include "dht.hxx"

// Based on
// * https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.cpp
// * https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/wiring_digital.c
// * https://maker.pro/custom/tutorial/how-to-take-analog-readings-with-an-avr-microcontroller
// * https://github.com/bendebled/avr-atmega328p-millis/blob/master/millis.c
// * https://appelsiini.net/2011/simple-usart-with-avr-libc/
// * https://github.com/adafruit/Adafruit_GPS/blob/master/Adafruit_GPS.h
// * https://github.com/arduino/ArduinoCore-avr/blob/master/libraries/SoftwareSerial/src/SoftwareSerial.cpp

namespace pins {
enum {
  kLcdRS = PD5,
  kLcdRW = PD6,
  kLcdEN = PD7,
  kLcdD4 = PB2,
  kLcdD5 = PB3,
  kLcdD6 = PB4,
  kLcdD7 = PB5,
  kGpsRx = PD0,
  kGpsTx = PD1
};
}

namespace compat {
namespace arduino {


void pinMode(std::uint8_t pin, std::uint8_t mode) {
  return;
}

}
}

static volatile char  uart_buffer[512];
static volatile char* uart_write_pos;
static volatile char* uart_read_pos;
static volatile bool  uart_buffer_overflow;

void uart_init() {
  // https://trolsoft.ru/en/uart-calc
  UBRR0H = 0x00;
  UBRR0L = 0x67;
  UCSR0A &= ~_BV(U2X0);

  // Possible sizes are 5-bit (000), 6-bit (001), 7-bit (010), 8-bit (011) and 9-bit (111).
  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);   // enable RX, TX, and RX interrupt
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data

  uart_write_pos = uart_buffer;
  uart_read_pos  = uart_buffer;
  uart_buffer_overflow = false;
}

void uart_init_57600() {
// TODO: Rewrite https://www.nongnu.org/avr-libc/user-manual/setbaud_8h_source.html
// with constexpr.
#if defined(BAUD)
# undef BAUD
#endif
#define BAUD 57600
#include <util/setbaud.h>
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
#if USE_2X
  UCSR0A |= _BV(U2X0);
#else
  UCSR0A &= ~_BV(U2X0);
#endif

  // Possible sizes are 5-bit (000), 6-bit (001), 7-bit (010), 8-bit (011) and 9-bit (111).
  UCSR0B |= _BV(RXEN0) | _BV(TXEN0);   // enable RX and TX
  UCSR0C |= _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data
}

bool uart_available() {
  return bit_is_set(UCSR0A, RXC0);
}

bool uart_wait() {
  loop_until_bit_is_set(UCSR0A, RXC0); // Wait until data exists.
  return true;
}

void uart_putchar(char c) {
  loop_until_bit_is_set(UCSR0A, UDRE0); // Wait until data register empty.
  UDR0 = c;
}

void uart_write(char const* ch) {
  while (*ch) {
    uart_putchar(*ch);
    ++ch;
  }
}

void uart_println() {
  uart_putchar('\r');
  uart_putchar('\n');
}

void uart_println(char const* ch) {
  while (*ch) {
    uart_putchar(*ch);
    ++ch;
  }

  uart_println();
}

// void uart_putchar(char c) {
//   UDR0 = c;
//   loop_until_bit_is_set(UCSR0A, TXC0); // Wait until transmission ready.
// }

char uart_getchar() {
  loop_until_bit_is_set(UCSR0A, RXC0); // Wait until data exists.
  return UDR0;
}

char uart_buf_available() {
  return uart_read_pos != uart_write_pos;

  if (uart_read_pos == uart_buffer + sizeof(uart_buffer)) {
    uart_read_pos = uart_buffer;
  }

  return (!uart_buffer_overflow && uart_read_pos < uart_write_pos)
      || (uart_buffer_overflow && uart_read_pos >= uart_write_pos);
}

char uart_buf_getchar() {
  if (uart_read_pos == uart_buffer + sizeof(uart_buffer)) {
    uart_read_pos = uart_buffer;
    uart_buffer_overflow = false;
  }

  char ch = *uart_read_pos;
  ++uart_read_pos;

  return ch;
}

char uart_getchar(bool* success) {
  loop_until_bit_is_set(UCSR0A, RXC0); // Wait until data exists.
  for (std::uint32_t timeout = us_to_clock_cycles(1000); timeout > 0; --timeout) {
    if (bit_is_set(UCSR0A, RXC0)) {
      *success = true;
      return UDR0;
    }
  }
  *success = false;
  return UDR0;
}

std::size_t uart_getline(char* ch, std::size_t n) {
  for (std::size_t i = 0; i < n; ++ch, ++i) {
    // bool is_read;
    // *ch = uart_getchar(&is_read);
    // if (!is_read) {
    //   break;
    // }
    *ch = uart_getchar();
    // if ((*ch != '$' && *ch != '*' && *ch != ',' && *ch != '.' && *ch != '-' && *ch != '\n' && *ch != '\0') &&
    //     !(*ch >= 'A' && *ch <= 'Z') &&
    //     !(*ch >= '0' && *ch <= '9')) {
    //   *ch = '#';
    // }
    if (*ch == '\0') {
      return i;
    }
    if (*ch == '\n') {
      *ch = '\0';
      return i;
    }
  }
  *ch = '\0';

  return n;
}

std::size_t uart_buf_getline(char* ch, std::size_t n) {
  for (std::size_t i = 0; i < n - 1; ++ch, ++i) {
    *ch = uart_buf_getchar();
    if (*ch == '\0') {
      return i;
    }
    if (*ch == '\n') {
      *(ch + 1) = '\0';
      return i;
    }
  }
  *ch = '\0';

  return n;
}

ISR(USART_RX_vect) {
  if (uart_write_pos == uart_buffer + sizeof(uart_buffer)) {
    uart_write_pos = uart_buffer;
    // uart_buffer_overflow = uart_write_pos < uart_read_pos;
  }

  char ch = uart_getchar();
  // if (!uart_buffer_overflow && uart_write_pos >= uart_read_pos) {
  if (uart_write_pos >= uart_read_pos) {
    *uart_write_pos = ch;
    ++uart_write_pos;
  } else {
    uart_buffer_overflow = true;
    // if (uart_write_pos < uart_read_pos) {
    //   *uart_write_pos = ch;
    //   ++uart_write_pos;
    // }
    *uart_write_pos = ch;
    ++uart_write_pos;
  }
}

namespace gps {

// Different commands to set the update rate from once a second (1 Hz) to 10 times a second (10Hz)
// Note that these only control the rate at which the position is echoed, to actually speed up the
// position fix you must also send one of the position fix rate commands below too.
//
// Once every 10 seconds, 100 millihertz.
static inline char const kPMTK_SET_NMEA_UPDATE_100_MILLIHERTZ[] = "$PMTK220,10000*2F";
// Once every 5 seconds, 200 millihertz.
static inline char const kPMTK_SET_NMEA_UPDATE_200_MILLIHERTZ[] = "$PMTK220,5000*1B";
static inline char const kPMTK_SET_NMEA_UPDATE_1HZ[]  = "$PMTK220,1000*1F";             //  1 Hz
static inline char const kPMTK_SET_NMEA_UPDATE_2HZ[]  = "$PMTK220,500*2B";              //  2 Hz
static inline char const kPMTK_SET_NMEA_UPDATE_5HZ[]  = "$PMTK220,200*2C";              //  5 Hz
static inline char const kPMTK_SET_NMEA_UPDATE_10HZ[] = "$PMTK220,100*2F";              // 10 Hz

// Position fix update rate commands.
//
// Once every 10 seconds, 100 millihertz.
static inline char const kPMTK_API_SET_FIX_CTL_100_MILLIHERTZ[] = "$PMTK300,10000,0,0,0,0*2C";
// Once every 5 seconds, 200 millihertz.
static inline char const kPMTK_API_SET_FIX_CTL_200_MILLIHERTZ[] = "$PMTK300,5000,0,0,0,0*18";
static inline char const kPMTK_API_SET_FIX_CTL_1HZ[] = "$PMTK300,1000,0,0,0,0*1C";     // 1 Hz
static inline char const kPMTK_API_SET_FIX_CTL_5HZ[] = "$PMTK300,200,0,0,0,0*2F";      // 5 Hz
// Can't fix position faster than 5 times a second!

static inline char const kPMTK_SET_BAUD_115200[] = "$PMTK251,115200*1F";  // 115200 bps
static inline char const kPMTK_SET_BAUD_57600[]  = "$PMTK251,57600*2C";   //  57600 bps
static inline char const kPMTK_SET_BAUD_9600[]   = "$PMTK251,9600*17";    //   9600 bps

// Turn on only the GPGLL sentence.
static inline char const kPMTK_SET_NMEA_OUTPUT_GLLONLY[] = "$PMTK314,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29";
// Turn on only the GPRMC sentence.
static inline char const kPMTK_SET_NMEA_OUTPUT_RMCONLY[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29";
// Turn on only the GPVTG.
static inline char const kPMTK_SET_NMEA_OUTPUT_VTGONLY[] = "$PMTK314,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29";
// Turn on just the GPGGA.
static inline char const kPMTK_SET_NMEA_OUTPUT_GGAONLY[] = "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29";
// Turn on just the GPGSA.
static inline char const kPMTK_SET_NMEA_OUTPUT_GSAONLY[] = "$PMTK314,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29";
// Turn on just the GPGSV.
static inline char const kPMTK_SET_NMEA_OUTPUT_GSVONLY[] = "$PMTK314,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0*29";
// Turn on GPRMC and GPGGA.
static inline char const kPMTK_SET_NMEA_OUTPUT_RMCGGA[]  = "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28";
// Turn on GPRMC, GPGGA and GPGSA.
static inline char const kPMTK_SET_NMEA_OUTPUT_RMCGGAGSA[] = "$PMTK314,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29";
// Turn on ALL THE DATA.
static inline char const kPMTK_SET_NMEA_OUTPUT_ALLDATA[] = "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28";
// Turn off output.
static inline char const kPMTK_SET_NMEA_OUTPUT_OFF[]     = "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28";

// To generate your own sentences, check out the MTK command datasheet and use a checksum calculator
// such as the awesome http://www.hhhh.org/wiml/proj/nmeaxor.html
//
// Start logging data.
static inline char const kPMTK_LOCUS_STARTLOG[]     = "$PMTK185,0*22";
// Stop logging data.
static inline char const kPMTK_LOCUS_STOPLOG[]      = "$PMTK185,1*23";
// Acknowledge the start or stop command.
static inline char const kPMTK_LOCUS_STARTSTOPACK[] = "$PMTK001,185,3*3C";
// Query the logging status.
static inline char const kPMTK_LOCUS_QUERY_STATUS[] = "$PMTK183*38";
// Erase the log flash data.
static inline char const kPMTK_LOCUS_ERASE_FLASH[]  = "$PMTK184,1*22";
// If flash is full, log will overwrite old data with new logs.
static inline char const LOCUS_OVERLAP  = 0;
// If flash is full, logging will stop.
static inline char const LOCUS_FULLSTOP = 1;

// Enable search for SBAS satellite (only works with 1Hz output rate).
static inline char const kPMTK_ENABLE_SBAS[] = "$PMTK313,1*2E";
// Use WAAS for DGPS correction data.
static inline char const kPMTK_ENABLE_WAAS[] = "$PMTK301,2*2E";

// Standby command & boot successful message.
static inline char const kPMTK_STANDBY[]         = "$PMTK161,0*28";
// Not needed currently.
static inline char const kPMTK_STANDBY_SUCCESS[] = "$PMTK001,161,3*36";
// Wake up.
static inline char const kPMTK_AWAKE[]           = "$PMTK010,002*2D";
// Ask for the release and version.
static inline char const kPMTK_Q_RELEASE[]       = "$PMTK605*31";
// Request for updates on antenna status.
static inline char const kPGCMD_ANTENNA[]        = "$PGCMD,33,1*6C";
// Don't show antenna status messages.
static inline char const kPGCMD_NOANTENNA[]      = "$PGCMD,33,0*6D";

struct GPS {
  std::uint8_t  hour;          // GMT hours
  std::uint8_t  minute;        // GMT minutes
  std::uint8_t  seconds;       // GMT seconds
  std::uint16_t milliseconds;  // GMT milliseconds
  std::uint8_t  year;          // GMT year
  std::uint8_t  month;         // GMT month
  std::uint8_t  day;           // GMT day

  char lat;                    // N/S
  char lon;                    // E/W
  char mag;                    // Magnetic variation direction

  // Fix quality:
  //   0 = Invalid
  //   1 = GPS fix (SPS)
  //   2 = DGPS fix
  //   3 = PPS fix
  //   4 = Real Time Kinematic
  //   5 = Float RTK
  //   6 = Estimated (dead reckoning) (2.3 feature)
  //   7 = Manual input mode
  //   8 = Simulation mode
  bool         fix;            // Have a fix?
  std::uint8_t fixquality;     // Fix quality (0, 1, 2 = Invalid, GPS, DGPS)
  std::uint8_t fixquality_3d;  // 3D fix quality (1, 3, 3 = Nofix, 2D fix, 3D fix)
  std::uint8_t satellites;     // Number of satellites in use

  float latitude;   // Floating point latitude value in degrees/minutes as received from the GPS (DDMM.MMMM)
  float longitude;  // Floating point longitude value in degrees/minutes as received from the GPS (DDDMM.MMMM)

  // Fixed point latitude and longitude value with degrees stored in units of 1/100000 degrees,
  // and minutes stored in units of 1/100000 degrees.  See pull #13 for more details:
  // https://github.com/adafruit/Adafruit-GPS-Library/pull/13
  int32_t latitude_fixed;   // Fixed point latitude in decimal degrees
  int32_t longitude_fixed;  // Fixed point longitude in decimal degrees

  float latitude_degrees;   // Latitude in decimal degrees
  float longitude_degrees;  // Longitude in decimal degrees
  float geoidheight;        // Diff between geoid height and WGS84 height
  float altitude;           // Altitude in meters above MSL
  float speed;              // Current speed over ground in knots
  float angle;              // Course in degrees from true north
  float magvariation;       // Magnetic variation in degrees (vs. true north)

  // Horizontal Dilution of Precision - relative accuracy of horizontal position
  float HDOP;
  // Vertical Dilution of Precision - relative accuracy of vertical position
  float VDOP;
  // Position Dilution of Precision - Complex maths derives a simple, single number for each kind of DOP
  float PDOP;

  bool parse_nmea(char const* nmea, std::size_t nmea_len) {
    //
    // https://www.gpsinformation.org/dale/nmea.htm
    //
    
    if (nmea_len < 3 || nmea[0] != '$' || nmea[1] != 'G' || (nmea[2] != 'P' && nmea[2] != 'N'))
      return false;

    char const* ast = strchr(nmea, '*');
    if (ast == nullptr)
      return false;

    std::uint16_t sum = parse_hex_digit(*(ast + 1)) * 16;
    sum += parse_hex_digit(*(ast + 2));

    for (char const* p = nmea + 1; p < ast; ++p) {
      sum ^= *p;
    }

    if (sum) {
      return false;  // bad checksum
    }

    if (nmea_len > 6 && nmea[3] == 'G' && nmea[4] == 'G' && nmea[5] == 'A') {
      //
      // Global Positioning System Fix Data
      //
      char const* p = &nmea[6];

      // time
      p = strchr(p, ',') + 1;
      parse_time(p);

      // latitude
      p = strchr(p, ',') + 1;
      parse_lat(p);
      p = strchr(p, ',') + 1;
      if (!parse_lat_dir(*p))
        return false;      

      // longitude
      p = strchr(p, ',') + 1;
      parse_lon(p);
      p = strchr(p, ',') + 1;
      if (!parse_lon_dir(*p))
        return false;

      // fixquality
      p = strchr(p, ',') + 1;
      if (*p != ',') {
        fixquality = atoi(p);
        fix = fixquality > 0;
      }

      // Number of satellites being tracked
      p = strchr(p, ',') + 1;
      if (*p != ',') {
        satellites = atoi(p);
      }

      // HDOP
      p = strchr(p, ',') + 1;
      if (*p != ',') {
        HDOP = atof(p);
      }

      // Altitude, Meters, above mean sea level
      p = strchr(p, ',') + 1;
      if (*p != ',') {
        altitude = atof(p);
      }
      // skip 'M'
      p = strchr(p, ',') + 1;

      // Height of geoid (mean sea level) above WGS84 ellipsoid
      p = strchr(p, ',') + 1;      
      if (*p != ',') {
        geoidheight = atof(p);
      }

      return true;
    }

    return false;
  }

  void parse_time(char const* p) {
    long int tm = atol(p);
    hour = tm / 10000;
    minute = (tm % 10000) / 100;
    seconds = tm % 100;

    if (p[6] == '.')
      milliseconds = atoi(p + 7);
    else
      milliseconds = 0;
  }
  
  void parse_lat(char const* p) {
    if (*p == ',')
      return;

    int degrees = 10 * (p[0] - '0') + (p[1] - '0');
    float minutes = atof(p + 2);

    latitude = degrees * 100 + minutes;
    latitude_degrees = degrees + minutes / 60.0f;
    latitude_fixed = latitude_degrees * 10000000;
  }

  constexpr bool parse_lat_dir(char c) {
    switch (c) {
      case 'S':
        lat = 'S';
        latitude_degrees *= -1.0f;
        latitude_fixed   *= -1;
        break;
      case 'N':
        lat = 'N';
        break;
      case ',':
        lat = '\0';
        break;
      default:
        return false;
    }
    return true;
  }

  void parse_lon(char const* p) {
    if (*p == ',')
      return;

    int degrees = 100 * (p[0] - '0') + 10 * (p[1] - '0') + (p[2] - '0');
    float minutes = atof(p + 3);

    longitude = degrees * 1000 + minutes;
    longitude_degrees = degrees + minutes / 60.0f;
    longitude_fixed = longitude_degrees * 10000000;
  }

  constexpr bool parse_lon_dir(char c) {
    switch (c) {
      case 'W':
        lon = 'W';
        longitude_degrees *= -1.0f;
        longitude_fixed   *= -1;
        break;
      case 'E':
        lon = 'E';
        break;
      case ',':
        lon = '\0';
        break;
      default:
        return false;
    }
    return true;
  }

  static constexpr std::uint8_t parse_hex_digit(char c) {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'A' && c <= 'F')
       return (c - 'A') + 10;
    if (c >= 'a' && c <= 'f')
       return (c - 'a') + 10;
    return 0;
  }
};

}

using namespace avr;

DHT dht(DDRD, PORTD, PORTD4);
LCD lcd(DDRD, PORTD, pins::kLcdEN, pins::kLcdRW, pins::kLcdRS,
        DDRB, PORTB, pins::kLcdD4, pins::kLcdD5, pins::kLcdD6, pins::kLcdD7);

int loop() {
  static bool is_odd = true;

  float temp = dht.read_temperature(true);
  float humidity = dht.read_humidity(false);

  //lcd.clear();
  lcd.home();

  static gps::GPS gps;
  static bool nmea_parsed = false;
  static char line[150];
  while (uart_buf_available()) {
    auto len = uart_buf_getline(line, sizeof(line));

    if (gps.parse_nmea(line, len)) {
      nmea_parsed = true;
    }
  }
  if (!nmea_parsed) {
    lcd.write("No NMEA");
  } else {
    snprintf(line, sizeof(line), "%.03f%c %.03f%c",
             fabs(gps.latitude_degrees), gps.lat ? gps.lat : ' ',
             fabs(gps.longitude_degrees), gps.lon ? gps.lon : ' ');
    // lcd.write(static_cast<int>(gps.satellites));
    // lcd.write(' ');
    // lcd.write(gps.latitude_fixed);
    // lcd.write(gps.lat);
    // lcd.write(' ');
    // lcd.write(gps.longitude_fixed);
    // lcd.write(gps.lon);
    lcd.write(line);
  }
  //
  // if (uart_buf_available()) {
  //   char line[150];
  //   uart_buf_getline(line, sizeof(line));
  //   line[16] = '\0';
  //   lcd.write(line);
  // }
  // while (uart_buf_available()) {
  //   char line[150];
  //   uart_buf_getline(line, sizeof(line));
  // }
  //
  // lcd.write((char const*)uart_buffer);
  // if (uart_available()) {
  //   char line[150];
  //   // line[0] = '\0';
  //   // line[1] = '\0';
  //   auto n = uart_getline(line, sizeof(line));
  //   line[16] = '\0';
  //   lcd.write(line);
  // } else {
  //   //lcd.write("No GPS data");
  // }

  lcd.set_cursor(1, 0);
  lcd.write(static_cast<unsigned int>(gps.satellites));
  lcd.write(' ');
  // lcd.write(temp, 1);
  if (isnan(temp)) {
    lcd.write("NAN");
  } else {
    lcd.write(char(int(temp / 10.0f) % 10 + '0'));
    lcd.write(char(int(temp / 1.0f) % 10 + '0'));
    lcd.write('.');
    lcd.write(char(int(temp / 0.1f) % 10 + '0'));
  }

  lcd.write('F');
  lcd.write(' ');

  if (isnan(humidity)) {
    lcd.write("NAN");
  } else {
    lcd.write(char(int(humidity / 10.0f) % 10 + '0'));
    lcd.write(char(int(humidity / 1.0f) % 10 + '0'));
    lcd.write('.');
    lcd.write(char(int(humidity / 0.1f) % 10 + '0'));
  }
  lcd.write('%');

  lcd.set_cursor(1, 15);
  lcd.write(is_odd ? '\x0' : ' ');

  is_odd = !is_odd;

  return 0;
}

int main() {
  millis_init();

  lcd.init();
  lcd.create_char(0, heart);
  lcd.create_char(1, smiley);
  lcd.clear();

  dht.begin();

  uart_init();
  // uart_println(gps::kPMTK_SET_BAUD_9600);
  uart_println(gps::kPMTK_SET_NMEA_OUTPUT_RMCGGA);
  uart_println(gps::kPMTK_SET_NMEA_UPDATE_1HZ);
  uart_println(gps::kPMTK_Q_RELEASE);

  sei();

  while (!loop()) {
    _delay_ms(500);
  }
}

#if 0
#include <math.h>

#include <LiquidCrystal.h>
#include <DHT.h>

// https://github.com/adafruit/Adafruit_GPS/blob/master/examples/GPS_SoftwareSerial_EchoTest/GPS_SoftwareSerial_EchoTest.ino
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

#if defined(USE_IR)

#include <IRremote.h>

// https://github.com/smallbeetw/arduinosketch/blob/master/Panasonic2KbroIRtrans/Panasonic2KbroIRtrans.ino
#define MAP_SIZE 16

#define PANASONIC_POWER   0xF61E2A57
#define PANASONIC_DTVTV   0x1C42833F
#define PANASONIC_OK      0xBB0ED9E1
#define PANASONIC_RETURN  0xD28EF217
#define PANASONIC_UP      0x4DE74847
#define PANASONIC_DOWN    0xB8781EF
#define PANASONIC_ONE     0xF7283C77
#define PANASONIC_TWO     0x757FB4DF
#define PANASONIC_THREE   0xB33B4597
#define PANASONIC_FOUR    0x3C03E507
#define PANASONIC_FIVE    0xE705551F
#define PANASONIC_SIX     0xA4A58EC7
#define PANASONIC_SEVEN   0xE2E45F7F
#define PANASONIC_EIGHT   0x6BACFEEF
#define PANASONIC_NINE    0xE88E91F
#define PANASONIC_ZERO    0x7D168BCF

struct TransfEntry {
  long unsigned int panasonicCode;       // Hash code of Panasonic
  unsigned int kbroRawCode[17];          // IR raw code of KBRO
};

static struct TransfEntry ir_transf_map[MAP_SIZE] = {
  {PANASONIC_POWER,  {200,900,200,750,250,750,200,750,200,750,250,2750,250,750,200,750,200}},
  /* DTV/TV to kbro Power */
  {PANASONIC_DTVTV,  {200,900,200,750,250,750,200,750,200,750,250,2750,250,750,200,750,200}},
  {PANASONIC_OK,     {250,850,250,1800,250,750,200,750,200,1050,200,1450,200,750,200,750,250}},
  {PANASONIC_RETURN, {250,850,250,1300,200,750,200,750,250,1000,200,2000,200,750,200,750,250}},
  /* Channel UP */
  {PANASONIC_UP,     {250,850,250,1000,250,700,250,750,200,750,250,2450,250,750,200,750,250}},
  /* Channel DOWN */
  {PANASONIC_DOWN,   {250,850,250,850,250,750,200,750,250,700,250,2600,250,750,200,750,250}},
  {PANASONIC_ONE,    {250,850,250,2650,200,750,200,750,250,750,200,900,200,750,200,750,250}},
  {PANASONIC_TWO,    {250,850,250,2450,250,750,200,750,250,700,250,1000,250,750,200,750,200}},
  {PANASONIC_THREE,  {200,900,200,2400,200,750,200,750,250,750,200,1150,200,750,250,750,200}},
  {PANASONIC_FOUR,   {200,900,200,2250,200,750,200,750,250,750,200,1300,200,750,250,700,250}},
  {PANASONIC_FIVE,   {250,850,250,2100,200,750,250,700,250,750,200,1400,250,750,200,750,250}},
  {PANASONIC_SIX,    {250,850,250,1950,200,750,250,750,200,750,200,1600,200,750,200,750,250}},
  {PANASONIC_SEVEN,  {200,900,200,1850,200,750,250,750,200,750,200,1700,250,700,250,750,200}},
  {PANASONIC_EIGHT,  {250,850,250,1650,250,750,200,750,250,750,200,1800,250,750,200,750,250}},
  {PANASONIC_NINE,   {200,900,250,1550,200,750,200,750,250,750,200,1950,250,750,200,750,200}},
  {PANASONIC_ZERO,   {200,900,200,2800,200,750,200,750,250,750,200,750,200,750,250,750,200}},
};

#endif

namespace pins {
enum {
  kLcdRS = 8,
  kLcdEN = 9,
  kLcdD4 = 4,
  kLcdD5 = 5,
  kLcdD6 = 6,
  kLcdD7 = 7,
  kLcdBL = 10,

  kSonicEcho = 2,
  kSonicTrigger = 2,

  kDHT = 12,

#if defined(USE_IR)
  kIrSend = 13,
  kIrRecv = 13,
#endif

  kGpsTx = 11,
  kGpsRx = 3
};
}

bool IsOdd = false;
unsigned int CycleI = 0;

LiquidCrystal Lcd(pins::kLcdRS, pins::kLcdEN, pins::kLcdD4, pins::kLcdD5, pins::kLcdD6, pins::kLcdD7);
DHT Dht(pins::kDHT, DHT11);
SoftwareSerial GpsSerial(pins::kGpsTx, pins::kGpsRx);
Adafruit_GPS Gps(&GpsSerial);

#if defined(USE_IR)
IRsend IrSend;
IRrecv IrRecv(pins::kIrRecv);
decode_results IrRecvResults;
#endif

void setup() {
  Lcd.begin(16, 2);

  pinMode(pins::kSonicTrigger, OUTPUT);
  digitalWrite(pins::kSonicTrigger, LOW);
  Serial.begin(9600);

  Dht.begin();

#if defined(USE_IR)
  IrSend.enableIROut(37);
  IrSend.mark(0);
  IrRecv.enableIRIn();
  IrRecv.blink13(true);
#endif

  Gps.begin(9600);
  Gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Gps.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  Gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  // Gps.sendCommand(PGCMD_ANTENNA);

  GpsSerial.println(PMTK_Q_RELEASE);
}

float measure_distance_once() {
  pinMode(pins::kSonicTrigger, OUTPUT);
  digitalWrite(pins::kSonicTrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pins::kSonicTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pins::kSonicTrigger, LOW);
  pinMode(pins::kSonicTrigger, INPUT);
  float duration = pulseIn(pins::kSonicEcho, HIGH);  // microseconds
  float distance = duration * (1125.33f * 12.0f * 1e-6f) / 2;  // inch/ms
  return distance;
}

float measure_distance(int interations = 5) {
  float distance = 0.0f;
  int distance_n = 0;
  for (; interations > 0; --interations) {
    float d = measure_distance_once();
    if (d > 0.79f && d < 78.74f) {
      distance += d;
      ++distance_n;
    }
  }
  if (!distance_n)
    return NAN;

  distance /= distance_n;
  return distance;
}

bool measure_dht(float& t_out, float& h_out) {
  float t = Dht.readTemperature(true);
  float h = Dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    return false;
  }

  float hif = Dht.computeHeatIndex(t, h, true);

  t_out = hif;
  h_out = h;

  return true;
}

void loop() {
  static unsigned long period950_millis = millis();

#if defined(USE_IR)
  if (IrRecv.decode(&IrRecvResults)) {
    Serial.print("IR reciever: decode_type=");
    Serial.print(IrRecvResults.decode_type, HEX);
    Serial.print(" value=");
    Serial.print(IrRecvResults.value, HEX);
    Serial.println();

    if (IrRecvResults.decode_type == UNKNOWN) {
      for (int i = 0; i < MAP_SIZE; i++) {
        if (IrRecvResults.value == ir_transf_map[i].panasonicCode) {
          IrSend.sendRaw(ir_transf_map[i].kbroRawCode, 17, 38);
          if (IrRecvResults.value == PANASONIC_POWER || IrRecvResults.value == PANASONIC_DTVTV)
            delay(400);
          else
            delay(30);
          break;
        }
      }
    }
    IrRecv.enableIRIn();
    IrRecv.resume();
  }
#endif

  static unsigned int GpsLatD = 0, GpsLatM = 0, GpsLonD = 0, GpsLonM = 0, GpsSatellites = 0;
  static char GpsLat = ' ', GpsLon = ' ';

  char c = Gps.read();
  do {
    if (0) Serial.write(c);
  } while (c = Gps.read());

  // If a sentence is received, we can check the checksum, parse it...
  if (Gps.newNMEAreceived()) {
    // A tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    auto lastNMEA = Gps.lastNMEA();

    // Serial.println(lastNMEA);   // this also sets the newNMEAreceived() flag to false

    if (!Gps.parse(lastNMEA)) {  // this also sets the newNMEAreceived() flag to false
      //Serial.print("could not parse NMEA: ");
      //Serial.println(lastNMEA);
    } else {
      //Serial.print("last NMEA: ");
      //Serial.println(lastNMEA);

      Serial.print(Gps.month);
      Serial.print("/");
      Serial.print(Gps.day);
      Serial.print("/");
      Serial.print(Gps.year);
      Serial.print(" ");
      if (Gps.hour < 10)
        Serial.print('0');
      Serial.print(Gps.hour, DEC);
      Serial.print(':');
      if (Gps.minute < 10)
        Serial.print('0');
      Serial.print(Gps.minute, DEC);
      Serial.print(':');
      if (Gps.seconds < 10)
        Serial.print('0');
      Serial.print(Gps.seconds, DEC);
      Serial.print('.');
      if (Gps.milliseconds < 10)
        Serial.print("00");
      else if (Gps.milliseconds > 9 && Gps.milliseconds < 100)
        Serial.print("0");
      Serial.println();
      Serial.print("fix=");
      Serial.print(Gps.fix);
      Serial.print(" fixquality=");
      Serial.print(Gps.fixquality);
      Serial.print(" fixquality_3d=");
      Serial.print(Gps.fixquality_3d);
      Serial.println();

      if (Gps.fix) {
        GpsLatD = static_cast<int>(Gps.latitude) / 100;
        GpsLatM = static_cast<int>(Gps.latitude) % 100;
        GpsLonD = static_cast<int>(Gps.longitude) / 100;
        GpsLonM = static_cast<int>(Gps.longitude) % 100;
        GpsLat = Gps.lat;
        GpsLon = Gps.lon;
        GpsSatellites = Gps.satellites;

        Serial.print("Gps {.latitude=");
        Serial.print(Gps.latitudeDegrees, 4);
        Serial.print(" ");
        Serial.print(Gps.lat);
        Serial.print(" .longitude=");
        Serial.print(Gps.longitudeDegrees, 4);
        Serial.print(" ");
        Serial.print(Gps.lon);
        Serial.print(" .speed (knots)=");
        Serial.print(Gps.speed);
        Serial.print(" .angle=");
        Serial.print(Gps.angle);
        Serial.print(" .altitude=");
        Serial.print(Gps.altitude);
        Serial.print(" .satellites=");
        Serial.print(Gps.satellites);
        Serial.println("}");

        Serial.print("Gps.details {.magvariation=");
        Serial.print(Gps.magvariation);
        Serial.print(" .mag=");
        Serial.print(Gps.mag);
        Serial.print(" .latitude=");
        Serial.print(Gps.latitude, 4);
        Serial.print(" ");
        Serial.print(Gps.lat);
        Serial.print(" .longitude=");
        Serial.print(Gps.longitude, 4);
        Serial.print(" ");
        Serial.print(Gps.lon);
        Serial.print(" .geoidheight=");
        Serial.print(Gps.geoidheight);
        Serial.print(" .HDOP=");
        Serial.print(Gps.HDOP);
        Serial.print(" .VDOP=");
        Serial.print(Gps.VDOP);
        Serial.print(" .PDOP=");
        Serial.print(Gps.PDOP);
        Serial.println("}");
      }
    }
  }

  if (millis() - period950_millis >= 950) {
    float distance = measure_distance();

    static float temperature = NAN, humidity = NAN;
    bool has_dht = measure_dht(temperature, humidity);

    Lcd.clear();

    Lcd.setCursor(0, 0);
    Lcd.print(GpsSatellites);
    Lcd.print(' ');
    Lcd.print(GpsLatD);
    Lcd.print(GpsLat);
    Lcd.print(GpsLatM);
    Lcd.print("' ");
    Lcd.print(GpsLonD);
    Lcd.print(GpsLon);
    Lcd.print(GpsLonM);
    Lcd.print('\'');

    Lcd.setCursor(0, 1);
    if (!isnan(temperature)) {
      Lcd.print(static_cast<unsigned>(temperature));
      Lcd.print("F");
    } else
      Lcd.print("#");

    Lcd.setCursor(5, 1);
    if (!isnan(humidity)) {
      Lcd.print(static_cast<unsigned>(humidity));
      Lcd.print("%");
    } else
      Lcd.print("#");

    Lcd.print(" D");
    if (!isnan(distance) && distance > 0.79 && distance < 78.74) {
      Lcd.print(distance, 2);
      Lcd.print("\"");
    } else
      Lcd.print("#");

    /*
    */
    period950_millis = millis();
  }

  delay(10);
}
#endif
