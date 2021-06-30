#include "Tlc5948.h"

Tlc5948     tlc; // PWM LED driver (using Hardware SPI)

void setup() {
    tlc.begin(); // sets up pins, default GS/DC/BC data and Func Ctrl bits

    tlc.setGsData(Channels::all,0xffff); // clear all data in the chip
    tlc.exchangeData(DataKind::gsdata);

    tlc.setDcData(Channels::all,0xff);
    tlc.setBcData(0x7f);
    Fctrls fSave = tlc.getFctrlBits();
    fSave &= ~(Fctrls::dsprpt_mask);
    fSave |= Fctrls::dsprpt_mode_1; // set atutodisplay repeat

    //fSave &= ~(Fctrls::espwm_mask);
    //fSave |= Fctrls::espwm_mode_1; // set ES PWM mode on, basically breaks up
                                     // long ON/OFF periods into 128 smaller segments
                                     // with even distribution

    tlc.setFctrlBits(fSave);
    tlc.exchangeData(DataKind::ctrldata);

    tlc.startBuiltinGsclk();
}

void loop() { // Blink

    tlc.setGsData(Channels::all,0x7fff);
    tlc.exchangeData(DataKind::gsdata);
    delay(1000);
    tlc.setGsData(Channels::all,0x0);
    tlc.exchangeData(DataKind::gsdata);
    delay(1000);

}
