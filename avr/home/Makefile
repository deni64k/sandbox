CXX=/Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avr-g++
OBJCOPY=/Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avr-objcopy
AVRDUDE=avrdude
CXXFLAGS=-DF_CPU=16000000UL -Os -mmcu=atmega328p -Wall -Werror -std=c++17
LDFLAGS=-Wl,-u,vfprintf -lprintf_flt -lm

all: main.hex

%.o: %.cxx
	$(CXX) $(CXXFLAGS) $< -c -o $@ -w

main.elf: main.o lcd.o dht.o millis.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ -w

main.hex: main.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

.PHONY: burn
burn: main.hex
	$(AVRDUDE) -b 19200 -c usbasp -p atmega328p -U flash:w:main.hex:i

# unbrick:
# 	/Users/denis/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino14/bin/avrdude -C/Users/denis/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino14/etc/avrdude.conf -v -v -v -v -patmega328p -cstk500v1 -P/dev/cu.usbmodem14501 -b19200 -e -U efuse:w:0x05:m -U hfuse:w:0xD6:m -U lfuse:w:0xFF:m
# 	/Users/denis/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino14/bin/avrdude -C/Users/denis/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino14/etc/avrdude.conf -v -v -v -v -patmega328p -cstk500v1 -P/dev/cu.usbmodem14501 -b19200 -e -U flash:w:/Applications/Arduino.app/Contents/Java/hardware/arduino/avr/bootloaders/atmega/ATmegaBOOT_168_atmega328_pro_8MHz.hex -U lock:w:0x0F:m

# atmega328p
# Were
# avrdude: safemode: Fuses OK (E:FD, H:DB, L:FF)
# For 8+ Mhz
# avrdude: safemode: Fuses OK (E:FF, H:DB, L:DE)
