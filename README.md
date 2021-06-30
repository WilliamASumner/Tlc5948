# Tlc5948

A library for interfacing with the TLC5948 LED Driver chip

## Background
The TLC5948 is a 16-channel PWM LED Driver chip with 16-bit grayscale (GS) PWM control, 7-bit dot correction (DC) PWM control and 7-bit global brightness (BC) current-based control. The chip uses an SPI-like interface with a latch to communicate and is programmable through a control buffer as well as a grayscale data buffer. For more details, see the [datasheet](https://www.ti.com/lit/ds/symlink/tlc5948a.pdf?ts=1617988879463&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FTLC5948A).

The library uses a fairly small set of methods to exchange control and grayscale information. Although the data itself is denoted as grayscale in the datasheet, it's just the brightness information for each channel, which means RGB leds can also be used with the chip.

## Supported Platforms
This project is currently only capable of supporting the Arduino Nano and Uno (or any other Atmega328p-compatible chip). Support for the Teensy 4.0 is planned. This library is mostly-compatible with the IDE, but is not official. I'm not sure if I'll ever get to documentation, but the design is fairly simple and a quick read through the two source files should be helpful. If you want to help out and submit a PR request to add support for your platform, feel free!

## Installation
To install this library, simply do:
```
git clone https://github.com/WilliamASumner/Tlc5948.git
```
and copy the `Tlc5948/` folder into your Arduino libraries folder. See this [article](https://www.arduino.cc/en/guide/libraries) for more information.

## Typical Blink Example
```c++
#include <Tlc5948.h>

Tlc5948 tlc;

void setup() {
    tlc.begin(); // sets up SPI and latch pins, default GS/DC/BC data and Func Ctrl bits, no data is sent

    tlc.setGsData(Channels::all,0xffff); // set all channels to max GS brightness
    tlc.exchangeData(DataKind::gsdata);

    tlc.setDcData(Channels::all,0x7f); // set all channels to max DC brightness
    tlc.setBcData(0x7f);               // and max BC brightness
 
    Fctrls fSave = tlc.getFctrlBits(); // get current (default) control settings
    fSave &= ~(Fctrls::dsprpt_mask);
    fSave |= Fctrls::dsprpt_mode_1;    // set autodisplay repeat, LED GS data will repeatedly be used

    fSave &= ~(Fctrls::espwm_mask);
    fSave |= Fctrls::espwm_mode_1;  // set ES PWM mode on, basically breaks up
                                    // long ON/OFF periods into 128 smaller segments
                                    // with even distribution across the range of GS values
                                    // In a picture: 
                                    // Normal PWM:      ------___------___------___
                                    // ES PWM:          --_--_--_--_--_--_--_--_--_
                                    // The end result is the same duty cycle, but a faster PWM freq
                                    // It also still uses the whole 16 bit resolution, and
                                    // adds on-time in a specific order to each periods for each bit
                                    // See the datasheet for more detail

    tlc.setFctrlBits(fSave);
    tlc.exchangeData(DataKind::ctrldata);

    tlc.startBuiltinGsclk();
}

void loop() {
    tlc.setGsData(Channels::all,0xffff);
    tlc.exchangeData(DataKind::gsdata);
    delay(500);

    tlc.setGsData(Channels::all,0x0);
    tlc.exchangeData(DataKind::gsdata);
    delay(500);
}

```
