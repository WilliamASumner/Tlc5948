#include "Tlc5948.h"

Tlc5948::Tlc5948() {
    sidStatus = SidFlags::NONE;
    funcControlBits = Fctrls::empty_bits;
    for (int i = 0; i < 32; i++) {
        gsDataBuf[i] = 0;
        ctrlDataBuf[i] = 0;
    }
}

// dot correction data, 7 bits per channel, 0 to 100%
void Tlc5948::setDcData(Channels channelMask, uint8_t value) {
    value &= 0x7f;
    for (int i = 0; i <= NUM_CHANNELS; i++) {
        if ((channelMask & Channels::chan_set) != Channels::chan_set) { // write only to selected channels
            channelMask >>= 1;
            continue; 
        }
        // writing 7 bits!
        int bitnum= i * 7;
        int bytenum= 31 - bitnum / 8; // offset from end of the array
        int align = bitnum % 8;
        ctrlDataBuf[bytenum] &= ~(0x7f << align) & 0xff;
        ctrlDataBuf[bytenum] |= (value << align) & 0xff;
        if (align > 1) {
            ctrlDataBuf[bytenum-1] &= ~(0x7f >> (8-align)) & 0xff;
            ctrlDataBuf[bytenum-1] |= (value >> (8-align)) & 0xff;
        }

        channelMask >>= 1;
    }
}

// Global brightness control data, 7 bits for all Channels (25% to 100%)
void Tlc5948::setBcData(uint8_t value) {
    value &= 0x7f;
    ctrlDataBuf[END_OF_DC_DATA] = value;
}

// Function Control, 18 bits
void Tlc5948::setFctrlBits(Fctrls f) {
    funcControlBits = f; // save this for easier modification
    unsigned long fbits = static_cast<unsigned long>(funcControlBits);
    ctrlDataBuf[END_OF_DC_DATA] &= ~(0xff << 7); // clear first bit value
    ctrlDataBuf[END_OF_DC_DATA] |= fbits << 7; // first bit gets put atop bc data (7 bits)
    ctrlDataBuf[END_OF_DC_DATA-1] = (fbits >> 1) & 0xff; // next 7 bits
    ctrlDataBuf[END_OF_DC_DATA-2] = (fbits >> 9) & 0xff; // ...
    ctrlDataBuf[END_OF_DC_DATA-3] = (fbits >> 17) & 0x01; // last 3 bits

}

// greyscale, pwm data, 16 bits per channel
// when blank bit of gs control reg is set (1), output is all 0's
// blank is set to 1 on startup, must write gs data before setting blank to 0
void Tlc5948::setGsData(Channels channelMask, uint16_t value) {
    for (int i = NUM_CHANNELS-1; i >= 0; i--) {
        if ((channelMask & Channels::out0) != Channels::out0) {
            channelMask >>= 1;
            continue;
        }
        gsDataBuf[i*2+1] = value & 0xFF;
        gsDataBuf[i*2] = (value >> 8) & 0xFF;
        channelMask >>= 1;
    }
}

inline void copyBuf(void* inBuf, void* outBuf, unsigned int size) {
    uint8_t* inByteBuf = static_cast<uint8_t*>(inBuf);
    uint8_t* outByteBuf = static_cast<uint8_t*>(outBuf);
    for (unsigned int i = 0; i < size; i++) {
        outByteBuf[i] = inByteBuf[i];
    }
}

// send data from either ctrl buff or gs data buff and read TLC5948 data
void Tlc5948::exchangeData(DataKind type, uint8_t numTlcs = 1) {
    SPI.beginTransaction(SPISettings(SPI_SPEED,BIT_ORDER,SPI_MODE));
    for (uint8_t i = 0; i < numTlcs; i++) {
        switch (type) {
            case DataKind::gsdata:
                copyBuf(gsDataBuf,spiBuf,32);
                //SPI.transfer(0x0);

                bitBangSpi0();

                break;
            case DataKind::ctrldata:
                copyBuf(ctrlDataBuf,spiBuf,32);
                //SPI.transfer(0x1);
                
                bitBangSpi1();

                break;
            default:
                break;
        }

        SPI.transfer(spiBuf,32);
    }
    SPI.endTransaction();
    pulseLatch(); // latch in the new data
}

SidFlags Tlc5948::getSidData(Channels& old, Channels& lsd, Channels& lod, bool refreshData) {
    if (refreshData) {
        exchangeData(DataKind::gsdata); // re-push in gsdata, pulling SidData out into spiBuf
        int delayMs = 0;
        Fctrls lattmg_bits = funcControlBits & (Fctrls::lattmg_mask);
        switch(lattmg_bits) {
            case Fctrls::lattmg_mode_17:
                delayMs = 17/PWM_FREQ*1000+1;
                break;
            case Fctrls::lattmg_mode_33:
                delayMs = 33/PWM_FREQ*1000+1;
                break;
            case Fctrls::lattmg_mode_65:
                delayMs = 64/PWM_FREQ*1000+1;
                break;
            default:
            case Fctrls::lattmg_mode_129:
                delayMs = 129/PWM_FREQ*1000+1;
                break;
        }
        delay(delayMs);
    }

    SidFlags flags = SidFlags::NONE;
    old = Channels::none;
    lsd = Channels::none;
    lod = Channels::none;

    for (int i = 1; i < 15; i += 2) { // every other byte is reserved, so just skip 'em
        uint16_t word = static_cast<uint16_t>(spiBuf[i]); // convert byte to 16 bit word so we can shift it
        if (word == 0) // skip empty words
            continue;
        switch(i) {
            case 13: // Misc bits
                if ((word >> 5) & 0x1)
                    flags |= SidFlags::ISF; // IREF is shorted
                if ((word >> 6) & 0x1) 
                    flags |= SidFlags::PTW; // Pre-thermal warning
                if ((word >> 7) & 0x1) 
                    flags |= SidFlags::TEF; // Thermal error flag
                break;
            case 11: // OLD bits 0..7
                old |= static_cast<Channels>(word);
                flags |= SidFlags::OLD; // Output leakage detected
                break;
            case 9: // OLD bits 8..15
                old |= static_cast<Channels>(word << 8);
                flags |= SidFlags::OLD; // Output leakage detected
                break;
            case 7: // LSD bits 0..7
                lsd |= static_cast<Channels>(word);
                flags |= SidFlags::LSD; // LED short detected
                break;
            case 5: // LSD bits 8..15
                lsd |= static_cast<Channels>(word << 8);
                flags |= SidFlags::LSD; // LED short detected
                break;
            case 3: // LOD bits 0..7
                lod |= static_cast<Channels>(word);
                flags |= SidFlags::LOD; // LED open detected
                break;
            case 1: // LOD bits 8..15
                lod |= static_cast<Channels>(word << 8);
                flags |= SidFlags::LOD; // LED open detected
                break;
            default:
                break;
        }
    }
    return flags;
}

void Tlc5948::begin() {
    // Note: driver must first send gs + dc/bc/fctrl data before it will turn on
    // this function just gets the buffers ready, 2+ calls to writeData are needed
    // to actually start the chip

    SPI.begin();

    // pin assignments
    pinMode(SSEL,OUTPUT); // slave select output -> prevent SPI slave mode
    pinMode(SIN,OUTPUT); // MOSI -> data to TLC5948
    pinMode(SOUT,INPUT);  // MISO -> data from TLC5948
    pinMode(SCLK,OUTPUT);  // SCLK -> SPI clk
    pinMode(LAT,OUTPUT);   // latch control
    pinMode(GSCLK,OUTPUT); // PWM clock

    setGsData(Channels::all,0xFFFF); // 100% brightness
    setDcData(Channels::all,0x7f); // all dot correction to 100%
    setBcData(0x7f); // global brightness to max

    Fctrls funcControls =   Fctrls::blank_mode_0    | // blank is set to 1 by chip, need to zero it out to use chip
                            Fctrls::dsprpt_mode_0   | // no async update color data
                            Fctrls::tmgrst_mode_0   | // no timing rst, lat is async
                            Fctrls::espwm_mode_0    | // no ES_PWM
                            Fctrls::lodvlt_mode_12v | // highest LOD vlt (1.2V)
                            Fctrls::lsdvlt_mode_065 | // highest LSD vlt (0.65*vcc)
                            Fctrls::lattmg_mode_17  | // 17 clks before we can latch error data
                            Fctrls::idmcur_mode_2ua | // 2ua on IDM
                            Fctrls::psmode_none;      // no power saving mode
                            //Fctrls::psmode_sclk     | // power off until sclk
                            //Fctrls::psmode_data     | // power off until new data
                            //Fctrls::psmode_noclk; // turn off internal GSCLK on power save mode

    setFctrlBits(funcControls);
}

void Tlc5948::readDeviceContents(uint8_t *bytes, int numBytes) {
    for (int i = 0; i < numBytes; i++) {
        bytes[i] = SPI.transfer(0x0);
    }
}

