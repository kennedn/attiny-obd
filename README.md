# attiny-obd

Display OBD statistics on a 16x2 LCD display using an attiny85.

This project hijacks the tx and rx serial lines on a conventional ELM327 OBD interface device to send and retreive data. This data is then displayed in a nice format on an LCD1602 via i2c.

<p align="center">
    <img src="./media/prototype.JPG" width="48%"/>
    <img src="./media/finished_and_mounted.JPG" width="48%"/>
</p>
<p align="center">
    <img src="./media/obd_wiring.JPG" width="48%"/>
    <img src="./media/avr_wiring.JPG" width="48%"/>
</p>

## Compiling and programming

An AVR programmer of choice can be used to program the attiny85, however a really easy way to achieve this is by using a spare raspberry pi as an SPI programmer. See the following blog post by Kevin Cuzner on how to set this up:

https://web.archive.org/web/20220629132626/http://kevincuzner.com/2013/05/27/raspberry-pi-as-an-avr-programmer/

Assuming that the linuxspi programmer is being used on a raspberry pi, the steps are:

### Configure the linuxspi programmer

The linuxspi programmer has a reset function that can drive the reset pin on the AVR device low to enable SPI programming. To enable this feature the following config should be placed at `/root/.avrduderc`:

```text
programmer
  id = "linuxspi";
  desc = "Use Linux SPI device in /dev/spidev*";
  type = "linuxspi";
  reset = 22;
;
```

### Connect Raspberry pi to attiny85

The following pinout should be followed:

| Rasp pi | attiny85 |
|---------|----------|
| GPIO 22 | PB5      |
| GPIO 10 | PB0      |  
| GPIO 09 | PB1      |  
| GPIO 11 | PB2      |  

### Configure the fuses on the device

The following changes need made to the stock fuse configuration for the attiny85 to run `avr-obd`:

- Clock divider bit CKDIV8 unset, this allows the attiny85 to run at 8Mhz
- Brown-out detection bits BODLEVEL[2:0] set to 4.3v, this prevents the 3.3v TX and RX lines of the ELM327 from  powering the device

The following `avrdude` command will set the correct bits on the fuses:

```bash
sudo avrdude -p t85 -P /dev/spidev0.0 -c linuxspi -b 30000 -U lfuse:w:0x62:m -U hfuse:w:0xdc:m -U efuse:w:0xff:m
```

Fuse values calculated using [Fuse Calculator](https://www.engbedded.com/fusecalc/).

### Run the Makefile

The makefile will build each *.c file under `src/` and then link them, it can also call avrdude to perform the spi copy.

```bash
cd src/avr-obd
make install
```
