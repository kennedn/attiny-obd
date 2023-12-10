BIN=avr-obd

MCU=attiny85
AVRDUDEMCU=t85
CPUFREQ=8000000
WRITESPEED=30000
PORT=/dev/spidev0.0
EEPROM_ERASE_COUNT=20
EEPROM_ERASE_STRING=$$(printf '0xFF,%.0s' $$(seq 1 $(EEPROM_ERASE_COUNT)))

CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS= -Os -Wall -DF_CPU=${CPUFREQ} -mmcu=${MCU}

SRC_DIR=src
BUILD_DIR=build
BUILD_DIRS=build build/lib
SRCS=$(wildcard $(SRC_DIR)/lib/*.c) $(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all checkdirs activate deactivate install clean size simulate

all: checkdirs ${BUILD_DIR}/${BIN}.hex

${BUILD_DIR}/%.o: $(SRC_DIR)/%.c | ${BUILD_DIR}
	${CC} ${CFLAGS} -c $< -o $@

${BUILD_DIR}/${BIN}.elf: ${OBJS} | ${BUILD_DIR}
	${CC} ${CFLAGS} -o $@ $^

${BUILD_DIR}/${BIN}.hex: ${BUILD_DIR}/${BIN}.elf
	${OBJCOPY} -O ihex $< $@


checkdirs: ${BUILD_DIRS}

activate:
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 1

deactivate:
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 0

install: ${BUILD_DIR}/${BIN}.hex size
	sudo avrdude -p ${AVRDUDEMCU} -P ${PORT} -c linuxspi -b ${WRITESPEED} -U flash:w:${BUILD_DIR}/${BIN}.hex -U eeprom:w:$(EEPROM_ERASE_STRING):m

eeprom:
	sudo avrdude -p ${AVRDUDEMCU} -P ${PORT} -c linuxspi -b ${WRITESPEED} -U eeprom:w:$(EEPROM_ERASE_STRING):m

fuse:
	sudo avrdude -p ${AVRDUDEMCU} -P ${PORT} -c linuxspi -b ${WRITESPEED} -U lfuse:r:-:i
clean:
	@rm -rf ${BUILD_DIR} ${BIN}.elf ${BIN}.hex

size: ${BUILD_DIR}/${BIN}.elf
	avr-size --format=avr --mcu=${MCU} $<

simulate:
	./tools/elm327_mimic.py

${BUILD_DIRS}:
	@mkdir -p $@
