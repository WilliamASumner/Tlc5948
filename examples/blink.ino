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


