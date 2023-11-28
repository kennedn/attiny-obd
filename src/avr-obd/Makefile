BIN=main

MCU=attiny85
AVRDUDEMCU=t85
CPUFREQ=8000000
WRITESPEED=30000

CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS= -Os -Wall -DF_CPU=${CPUFREQ} -mmcu=${MCU}
SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)
PORT=/dev/spidev0.0

EEPROM_ERASE_COUNT=3
EEPROM_ERASE_STRING=$$(printf '0x00,%.0s' $$(seq 1 $(EEPROM_ERASE_COUNT)))

${BIN}.hex: ${BIN}.elf
	${OBJCOPY} -O ihex $< $@

${BIN}.elf: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^

activate:
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 1

deactivate:
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 0

install: ${BIN}.hex size
	sudo avrdude -p ${AVRDUDEMCU} -P ${PORT} -c linuxspi -b ${WRITESPEED} -U flash:w:${BIN}.hex -U eeprom:w:$(EEPROM_ERASE_STRING):m

clean:
	rm -f ${BIN}.elf ${BIN}.hex ${OBJS}

size:
	    avr-size --format=avr --mcu=attiny85 ${BIN}.elf

simulate:
	./../../tools/elm327_mimic.py

