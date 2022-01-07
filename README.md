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

    tlc.setDcData(Channels::all,0x7f); // set all channels to max DC brightness
    tlc.setBcData(0x7f);               // and max BC brightness
 
    Fctrls fSave = tlc.getFctrlBits(); // get current (default) control settings
    fSave &= ~(Fctrls::dsprpt_mask);
    fSave |= Fctrls::dsprpt_mode_1;    // set autodisplay repeat, LED GS data will repeatedly be used
    fSave &= ~(Fctrls::tmgrst_mask);
    fSave |= Fctrls::tmgrst_mode_1;    // set autodisplay repeat, LED GS data will repeatedly be used

    fSave &= ~(Fctrls::espwm_mask);
    fSave |= Fctrls::espwm_mode_1;  // set ES PWM mode on, basically breaks up
                                    // long ON/OFF periods into 128 smaller segments
                                    // with even (but random) distribution across
                                    // the range of GS values.
                                    // In a picture: 
                                    // Normal PWM:      ------___------___------___
                                    // ES PWM:          --_--_--_--____--_--_--_--_
                                    // The end result is the same duty cycle,
                                    // but a faster PWM freq
                                    // It also still uses the whole 16 bit
                                    // resolution, and adds on-time in a
                                    // specific order to each periods for each
                                    // bit. See the datasheet for more detail.

    tlc.setFctrlBits(fSave);
    tlc.writeControlBufferSPI();
    tlc.startBuiltinGsclk(); // pin 2 Teensy, pin 9 Arduino
}

uint16_t blackColor[3] = {0x0000,0x0000,0x0000};
uint16_t redColor[3] = {0x00ff,0x0000,0x0000}; // this may be a different color
                                       // based on how your TLCs are connected.
                                       // array[0] will be the first value sent,
                                       // then array[1], etc.
void loop() {
    tlc.writeGsBufferSPI16(redColor,3); // writeGsBuffer will "tile" array across all channels of TLCs specified at the tlc.begin()
    delay(500);

    tlc.writeGsBufferSPI16(blackColor,3);
    delay(500);
}

```

## Notes
* `tlc.writeControlBuffer` is the only function that can be used to write control buffer data to individual TLC5948s in a chain. The reason is that the SPI versions of those functions only work with 16-bit chunks, and because of the strange 257-bit register inside of the TLC5948, we have to "know" the number of TLC's we're about to write to in order to get the data to enter each TLC at the right offset. The SPI version therefore can only duplicate one version of the control data across all TLCs. That's ok though, because even though the bitbanged version is slow, it's not *horrible* (depending on your mcu) and control data is mostly set-and-forget anyway. You're not going to want to animate the LEDs' brightness through the brightness control or dot correction because you can mostly get the same functionality (BC and DC are current limiting rather than PWM, but the end effect is the same) out of the GS buffer. If someone is brave enough to find an elegant solution to this problem feel free to send a pull request.
