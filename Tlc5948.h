#ifndef TLC5948_LIB_H
#define TLC5948_LIB_H
#include <SPI.h>

// pin assignments; todo replace with enum maybe
#ifdef ARDUINO_TEENSY40 // Teensy version
#warning "Using Teensy Pin Definitions"
const int LAT = 7;   // latch control
const int GSCLK = 2; // pwm clock, using pin 2
const int SSEL = 10; // slave select, HW SS, not needed
const int SIN = 11;   // serial data input (to Tlc5948)
const int SOUT = 12;  // serial data output (from Tlc5948) 
const int SCLK = 13;  // serial data clock
#else // assume Arduino Nano
#warning "Using Arduino Nano Pin Definitions"
const int LAT = 3;   // latch control, using D3
const int GSCLK = 9; // pwm clock, using D9 w/Fast PWM (~8Mhz)
const int SSEL = 10; // slave select, HW SS, not needed
const int SIN = 11;   // serial data input (to Tlc5948) HW MOSI, using D11
const int SOUT = 12;  // serial data output (from Tlc5948)  HW MISO, using D12
const int SCLK = 13;  // HW SCLK, using D13
#endif // ifdef ARDUINO_TEENSY40

// SPI settings
const uint32_t SPI_SPEED = 1000000;// 33mhz listed on data sheet, 1Mhz seems to work
const unsigned int BIT_ORDER = MSBFIRST;
const unsigned int SPI_MODE = SPI_MODE0;
const int NUM_CHANNELS = 16;
const uint32_t PWM_FREQ = 8000000; // max speed from fast PWM mode
const uint16_t MAX_BRIGHTNESS = 0xffff;
const uint16_t MIN_BRIGHTNESS = 0x0;
const uint16_t END_OF_DC_DATA = 31-NUM_CHANNELS * 7 / 8;

// led open, led short, output leakage, iref short flag, pre-thermal warning, thermal error flag
// SidFlags = BADPARSE TEF PTW ISF OLD LSD LOD
enum class SidFlags { NONE=0,LOD=1,LSD=2,OLD=4,ISF=8,PTW=16,TEF=32,BADPARSE=64 };

inline SidFlags operator|(SidFlags a, SidFlags b) {
    return static_cast<SidFlags>(static_cast<int>(a) | static_cast<int>(b));
}
inline SidFlags operator&(SidFlags a, SidFlags b) {
    return static_cast<SidFlags>(static_cast<int>(a) & static_cast<int>(b));
}
inline void operator&=(SidFlags& a, SidFlags b) {
    a = static_cast<SidFlags>(static_cast<int>(a) & static_cast<int>(b));
}
inline void operator|=(SidFlags& a, SidFlags b) {
    a = static_cast<SidFlags>(static_cast<int>(a) | static_cast<int>(b));
}
inline SidFlags operator~(SidFlags a) {
    return static_cast<SidFlags>(~static_cast<int>(a));
}

inline void printSidFlags(SidFlags s) {
    if ((s & SidFlags::TEF) != SidFlags::NONE)
        Serial.print(" TEF ");
    if ((s & SidFlags::PTW) != SidFlags::NONE)
        Serial.print(" PTW ");
    if ((s & SidFlags::ISF) != SidFlags::NONE)
        Serial.print(" ISF ");
    if ((s & SidFlags::OLD) != SidFlags::NONE)
        Serial.print(" OLD ");
    if ((s & SidFlags::LSD) != SidFlags::NONE)
        Serial.print(" LSD ");
    if ((s & SidFlags::LOD) != SidFlags::NONE)
        Serial.print(" LOD ");
    Serial.println();
}



enum class Channels : uint16_t { // Channel masks
    none =     0x0000,
    chan_set = 0x0001,
    out0 =     0x0001,
    out1 =     0x0002,
    out2 =     0x0004,
    out3 =     0x0008,
    out4 =     0x0010,
    out5 =     0x0020,
    out6 =     0x0040,
    out7 =     0x0080,
    out8 =     0x0100,
    out9 =     0x0200,
    out10 =    0x0400,
    out11 =    0x0800,
    out12 =    0x1000,
    out13 =    0x2000,
    out14 =    0x4000,
    out15 =    0x8000,
    even =     0xaaaa,
    odd =      0x5555,
    all =      0xffff,
    upper8 =   0xff00,
    lower8 =   0x00ff,
};

inline Channels operator|(Channels a, Channels b) {
    return static_cast<Channels>(static_cast<int>(a) | static_cast<int>(b));
}
inline Channels operator&(Channels a, Channels b) {
    return static_cast<Channels>(static_cast<int>(a) & static_cast<int>(b));
}
inline void operator&=(Channels& a, Channels b) {
    a = static_cast<Channels>(static_cast<int>(a) & static_cast<int>(b));
}
inline void operator|=(Channels& a, Channels b) {
    a = static_cast<Channels>(static_cast<int>(a) | static_cast<int>(b));
}
inline Channels operator~(Channels a) {
    return static_cast<Channels>(~static_cast<int>(a));
}
inline Channels operator>>(Channels a, int b) {
    return static_cast<Channels>(static_cast<int>(a) >> b);
}
inline Channels operator<<(Channels a, int b) {
    return static_cast<Channels>(static_cast<int>(a) << b);
}
inline void operator>>=(Channels& a, int b) {
    a = static_cast<Channels>(static_cast<int>(a) >> b);
}
inline void operator<<=(Channels& a, int b) {
    a = static_cast<Channels>(static_cast<int>(a) << b);
}
inline void printChannels(Channels c) {
    Serial.println(static_cast<unsigned int>(c),HEX);
}

enum class Fctrls : uint32_t { // function control masks and values
    blank_mask =      0x00001, // turns off outputs
    dsprpt_mask =     0x00002, // auto display repeat, DC,BC,GS data updated async
    tmgrst_mask =     0x00004, // allows LAT to control timing (new data interrupts)
    espwm_mask =      0x00008,
    lodvlt_mask =     0x00030,
    lsdvlt_mask =     0x000c0,
    lattmg_mask =     0x00300,
    idmena_mask =     0x00400,
    idmrpt_mask =     0x00800,
    idmcur_mask =     0x03000,
    oldena_mask =     0x04000,
    psmode_mask =     0x38000,

    psmode_none =       0x00000,
    psmode_sclk =       0x08000,
    psmode_data =       0x10000,
    psmode_noclk =      0x20000,
    oldena_mode_0 =     0x00000,
    oldena_mode_1 =     0x04000,
    idmcur_mode_2ua =   0x00000,
    idmcur_mode_10ua =  0x01000,
    idmcur_mode_20ua =  0x02000,
    idmcur_mode_1ma =   0x03000,
    idmrpt_mode_0=      0x00000,
    idmrpt_mode_1=      0x00800,
    idmena_mode_0=      0x00000,
    idmena_mode_1=      0x00400,
    lattmg_mode_17 =    0x00000,
    lattmg_mode_33 =    0x00100,
    lattmg_mode_65 =    0x00200,
    lattmg_mode_129 =   0x00300,
    lsdvlt_mode_035 =   0x00000,
    lsdvlt_mode_045 =   0x00040,
    lsdvlt_mode_055 =   0x00080,
    lsdvlt_mode_065 =   0x000c0,
    lodvlt_mode_03v =   0x00000,
    lodvlt_mode_06v =   0x00010,
    lodvlt_mode_09v =   0x00020,
    lodvlt_mode_12v =   0x00030,
    espwm_mode_0  =     0x00000,
    espwm_mode_1  =     0x00008,
    tmgrst_mode_0  =    0x00000,
    tmgrst_mode_1  =    0x00004,
    dsprpt_mode_0 =     0x00000,
    dsprpt_mode_1 =     0x00002,
    blank_mode_0 =      0x00000,
    blank_mode_1 =      0x00001,

    empty_bits   =      0x00000,
    full_bits    =      0xfffff,
};

inline Fctrls operator|(Fctrls a, Fctrls b) {
    return static_cast<Fctrls>(static_cast<int>(a) | static_cast<int>(b));
}
inline Fctrls operator&(Fctrls a, Fctrls b) {
    return static_cast<Fctrls>(static_cast<int>(a) & static_cast<int>(b));
}
inline void operator|=(Fctrls& a, Fctrls b) {
    a = static_cast<Fctrls>(static_cast<int>(a) | static_cast<int>(b));
}
inline void operator&=(Fctrls& a, Fctrls b) {
    a = static_cast<Fctrls>(static_cast<int>(a) & static_cast<int>(b));
}
inline Fctrls operator~(Fctrls a) {
    return static_cast<Fctrls>(~static_cast<int>(a));
}
inline int operator>>(Fctrls a, int shift) {
    return static_cast<int>(a) >> shift;
}

// Datasheet will help tremendously with decoding this
inline void printFctrls(Fctrls f) {
    Serial.print("Blank: 0x");
    Serial.println((f & Fctrls::blank_mask)>>0,HEX);

    Serial.print("Autodisplay repeat: 0x");
    Serial.println((f & Fctrls::dsprpt_mask)>>1,HEX);

    Serial.print("Timing reset: 0x");
    Serial.println((f & Fctrls::tmgrst_mask)>>2,HEX);

    Serial.print("ESPWM: 0x");
    Serial.println((f & Fctrls::espwm_mask)>>3,HEX);

    Serial.print("LOD voltage: 0x");
    Serial.println((f & Fctrls::lodvlt_mask)>>4,HEX);

    Serial.print("LSD voltage: 0x");
    Serial.println((f & Fctrls::lsdvlt_mask)>>6,HEX);

    Serial.print("Lat Timing: 0x");
    Serial.println((f & Fctrls::lattmg_mask)>>8,HEX);

    Serial.print("Idm Enable: 0x");
    Serial.println((f & Fctrls::idmena_mask)>>10,HEX);

    Serial.print("Idm repeat: 0x");
    Serial.println((f & Fctrls::idmena_mask)>>11,HEX);

    Serial.print("Idm current: 0x");
    Serial.println((f & Fctrls::idmrpt_mask)>>12,HEX);

    Serial.print("OLD enable: 0x");
    Serial.println((f & Fctrls::oldena_mask)>>14,HEX);

    Serial.print("PS mode: 0x");
    Serial.println((f & Fctrls::psmode_mask)>>15,HEX);
}

class Tlc5948 {
    public:
        void setDcData(Channels,uint8_t);
        void setBcData(uint8_t);
        void setFctrlBits(Fctrls);

        //void exchangeData(DataKind, uint8_t numTlcs = 1); // SPI mode
        void writeControlBuffer(uint8_t numTlcs = 1);
        void writeGsBuffer(uint8_t*buff, uint16_t numBytes, bool = false);
        void emptyGsBuffer(uint16_t numBytes);
        void fillGsBuffer(uint16_t numBytes, uint8_t val);
        SidFlags getSidData(Channels&,Channels&,Channels&,bool = false);

        void startBuiltinGsclk();
        void stopBuiltinGsclk();
        void pulseLatch();
        Fctrls getFctrlBits();
        void printGsDataBuf();
        void printSpiBuf();
        void printCtrlDataBuf();

        void begin(void);

        void readDeviceContents(uint8_t*,int);

        Tlc5948();

    private:
        SidFlags sidStatus;
        Fctrls funcControlBits;
        uint8_t ctrlDataBuf[32] = { 0 };

};

#if ARDUINO_TEENSY40
// use fast versions 
inline void pulse_high(int pinNum) { // ___----___
    digitalWriteFast(pinNum,HIGH);
    digitalWriteFast(pinNum,LOW);
}

inline void pulse_low(int pinNum) { // ---____---
    digitalWriteFast(pinNum,LOW);
    digitalWriteFast(pinNum,HIGH);
}
#else // use widely supported functions
inline void pulse_high(int pinNum) { // ___----___
    digitalWrite(pinNum,HIGH);
    digitalWrite(pinNum,LOW);
}

inline void pulse_low(int pinNum) { // ---____---
    digitalWrite(pinNum,LOW);
    digitalWrite(pinNum,HIGH);
}
#endif

inline void Tlc5948::pulseLatch() {
    pulse_high(LAT);
}

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#warning "Using Arduino Nano SPI Bit Bang"
inline void bitBangSpi1() {
    noInterrupts();

    SPCR &= ~_BV(SPE); // disable hw SPI (SPI.begin() stops us from writing to MOSI)

    // Bit bang a '1'
    PORTB |= 0b00001000; // set PB3/D11/MOSI high
    PORTB |= 0b00100000; // set PB5/D13/SCLK high
    PORTB &= 0b11011111; // set PB5/D13/SCLK low

    SPCR |= _BV(SPE); // restore hw SPI

    interrupts();
}

inline void bitBangSpi0() {
    noInterrupts();

    SPCR &= ~_BV(SPE); // disable hw SPI

    // Bit bang a '0'
    PORTB &= 0b11110111; // set PB3/D11/MOSI low
    PORTB |= 0b00100000; // set PB5/D13/SCLK high
    PORTB &= 0b11011111; // set PB5/D13/SCLK low

    SPCR |= _BV(SPE); // restore hw SPI

    interrupts();
}

inline void bitBang1() {

    digitalWrite(SIN,HIGH);
    digitalWrite(SCLK,HIGH);
    digitalWrite(SCLK,LOW);
    digitalWrite(SIN,LOW);

}

inline void bitBang0() {

    digitalWrite(SIN,LOW);
    digitalWrite(SCLK,HIGH);
    digitalWrite(SCLK,LOW);

}

inline void Tlc5948::startBuiltinGsclk() {
    // On Arduino Nano
    // timer 0 -> A: 6 B: 5 
    // timer 1 -> A: 9 B: 10 * using this timer
    // timer 2 -> A: 3 B: 11

    // From https://withinspecifications.30ohm.com/2014/02/20/Fast-PWM-on-AtMega328/
    // and atmega328p datasheet

    // To set appropriate mode for PWM we need three settings enabled:
    // TCCRXA - PWM mode + output CLEAR invert/non-invert
    // Fast Pwm Mode(counts up and resets to 0, and changes output on OCR0X val)
    // \- We do this by settings WGM0[1:0] in TCCR0A to 1 and WGM02 TCCR0B to 1
    // \- WGM02 in TCCR0B also specifies that reset to 0 happens at OCR0A value
    // \- and not at TOP (255 for timer 0, 65536 for timer 1)
    // set COM0X1 bits to 1
    // \- sets output to clear on match and start from BOTTOM (non-inverting)
    ICR1 = 1; // according to datasheet this works well for static duty as TOP
              // using 1 as TOP gives 1 bit resolution but 8Mhz max frequency

    // enable A and B, using OCR1A TOP
    //TCCRXA = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11) | _BV(WGM10);
    //enable just A, using ICR1 as TOP (leave WGM10 as 0)
    TCCR1A = _BV(COM1A1) | _BV(WGM11);

    // TCCRXB - Timer control Reg b
    // controls: clock prescaler (and upper bits of WGM)
    // for timer1: CS0[2:0] = 001 -> prescaler = 1 (produces 8Mhz signal)
    //       note: WGM02   =  1 -> set Fast PWM mode
    //       other prescalers: 010 -> prescaler = 8 (produces 1Mhz signal, cleaner than the 8Mhz)
    
    // Using _BV(CS11) produces slower 1Mhz signal, but it's cleaner than the 8Mhz one
    // this signal can be used if the ESPWM mode
    TCCR1B =  _BV(WGM13) | _BV(WGM12) | _BV(CS10);

    OCR1A = 0;
}

inline void Tlc5948::stopBuiltinGsclk() {
    TCCR1A &= ~(_BV(COM1A1)); // disconnect A
}
#elif defined(ARDUINO_TEENSY40)
inline void bitBangSpi1() {

    SPI.end();
    pinMode(SIN,OUTPUT);
    pinMode(SCLK,OUTPUT);

    digitalWriteFast(SIN,HIGH);
    digitalWriteFast(SCLK,HIGH);
    digitalWriteFast(SCLK,LOW);
    digitalWriteFast(SIN,LOW);

    SPI.begin(); // reenable SPI

}

inline void bitBangSpi0() {

    SPI.end(); // disable SPI
    pinMode(SIN,OUTPUT);
    pinMode(SCLK,OUTPUT);


    digitalWriteFast(SIN,LOW);
    digitalWriteFast(SCLK,HIGH);
    digitalWriteFast(SCLK,LOW);

    SPI.begin(); // reenable SPI

}

inline void bitBang1() {

    digitalWriteFast(SIN,HIGH);
    digitalWriteFast(SCLK,HIGH);
    digitalWriteFast(SCLK,LOW);
    digitalWriteFast(SIN,LOW);

}

inline void bitBang0() {

    digitalWriteFast(SIN,LOW);
    digitalWriteFast(SCLK,HIGH);
    digitalWriteFast(SCLK,LOW);

}

inline void Tlc5948::startBuiltinGsclk() {
    analogWriteFrequency(GSCLK, 8000000); // 8Mhz clock should be fast enough
    analogWrite(GSCLK,127);
}

inline void Tlc5948::stopBuiltinGsclk() {
    analogWrite(GSCLK,0);
}

#else
#warning "Non-tested platform, feel free to add a PR on GitHub!"
inline void bitBangSpi1() {
    SPI.end(); 

    digitalWrite(SIN,HIGH);
    digitalWrite(SCLK,HIGH);
    digitalWrite(SCLK,LOW);
    digitalWrite(SIN,LOW);

    SPI.begin();
}

inline void bitBangSpi0() {
    SPI.end();

    digitalWrite(SIN,LOW);
    digitalWrite(SCLK,HIGH);
    digitalWrite(SCLK,LOW);

    SPI.begin();
}

inline void bitBang1() {

    digitalWrite(SIN,HIGH);
    digitalWrite(SCLK,HIGH);
    digitalWrite(SCLK,LOW);
    digitalWrite(SIN,LOW);

}

inline void bitBang0() {

    digitalWrite(SIN,LOW);
    digitalWrite(SCLK,HIGH);
    digitalWrite(SCLK,LOW);

}

inline void Tlc5948::startBuiltinGsclk() {
    analogWrite(GSCLK,127);
}

inline void Tlc5948::stopBuiltinGsclk() {
    analogWrite(GSCLK,0);
}
#endif

inline void printBuf(uint8_t* buf, int size) {
    for (int i = 0; i < size; i++) {
        Serial.print("0x");
        if (buf[i] <= 15)
            Serial.print("0");
        Serial.print(buf[i],HEX);
        if (i % 8 == 7)
            Serial.println();
        else
            Serial.print(" ");
    }
    Serial.println();
}

inline void Tlc5948::printCtrlDataBuf() {
    printBuf(ctrlDataBuf,32);
}

inline Fctrls Tlc5948::getFctrlBits() {
    return funcControlBits;
}

#endif
