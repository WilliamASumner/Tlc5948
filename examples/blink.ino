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


