#include "Tlc5948.h"

Tlc5948::Tlc5948() {
    sidStatus = SidFlags::NONE;
    funcControlBits = Fctrls::empty_bits;
    for (int i = 0; i < 32; i++) {
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

inline void copyBuf(void* inBuf, void* outBuf, unsigned int size) {
    uint8_t* inByteBuf = static_cast<uint8_t*>(inBuf);
    uint8_t* outByteBuf = static_cast<uint8_t*>(outBuf);
    for (unsigned int i = 0; i < size; i++) {
        outByteBuf[i] = inByteBuf[i];
    }
}

// send data from ctrl buff
void Tlc5948::writeControlBuffer(uint8_t numTlcs) {
    for (uint8_t i = 0; i < numTlcs; i++) {
        bitBang1();
        for (uint8_t j = 0; j < 32; j++ ) {
            shiftOut(SIN,SCLK,BIT_ORDER,ctrlDataBuf[j]);
        }
    }
    pulseLatch(); // latch in the new data
}

// send data from gsdata buff
void Tlc5948::writeGsBuffer(uint8_t* buf, uint16_t numBytes, bool padding) {
    for (int i = 0; i < numBytes; i++) {
        if (i % 32 == 0) bitBang0();
        shiftOut(SIN,SCLK,BIT_ORDER,buf[i]);
    }

    if (padding) {
        for (int i = 0; i < 32 - (numBytes % 32); i++) {
            shiftOut(SIN,SCLK,BIT_ORDER,0);
        }
    }
    pulseLatch();
}

// write an empty GS data buffer (good for initializing)
void Tlc5948::emptyGsBuffer(uint16_t numBytes) {
    for (int i = 0; i < numBytes; i++) {
        if (i % 32 == 0) bitBang0();
        shiftOut(SIN,SCLK,BIT_ORDER,0);
    }
    pulseLatch();
}

// write a  GS data buffer with all one value (for a test blink)
void Tlc5948::fillGsBuffer(uint16_t numBytes, uint8_t val) {
    for (int i = 0; i < numBytes; i++) {
        if (i % 32 == 0) bitBang0();
        shiftOut(SIN,SCLK,BIT_ORDER,val);
    }
    pulseLatch();
}

// Need to find a way to unify shiftOut cmd and shiftIn to read back data
/*SidFlags Tlc5948::getSidData(Channels& old, Channels& lsd, Channels& lod, bool refreshData) {
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
}*/

void Tlc5948::begin() {
    // Note: driver must first send gs + dc/bc/fctrl data before it will turn on
    // this function just gets the buffers ready, 2+ calls to writeData are needed
    // to actually start the chip

    // pin assignments
    pinMode(SIN,OUTPUT); // MOSI -> data to TLC5948
    pinMode(SOUT,INPUT);  // MISO -> data from TLC5948
    pinMode(SCLK,OUTPUT);  // SCLK -> SPI clk

    pinMode(SSEL,OUTPUT); // slave select output -> prevent SPI slave mode
    pinMode(LAT,OUTPUT);   // latch control
    pinMode(GSCLK,OUTPUT); // PWM clock

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
        bytes[i] = shiftIn(SOUT,SCLK,BIT_ORDER);
    }
}

