/**
 * Tennsy 3.2 configures AD5930 DDS chip to output a fixed-frequency 100KHz sine wave
 * By Yang Zhang 
 * License: Do whatever you want with this. No warranty.
 */

#include <SPI.h>

#define EXTERN_CLOCK_FREQ 50000000
#define CTRL_REG_MASK 0x3
#define FSYNC 16 // PORTB_0
#define CTRL 17 // PORTB_1
#define SCLK 13 // MOSI and SCK are pin 11 and 13 on Teensy 3.2
#define SDATA 11

// For the control register
typedef enum mode{
  SAW_SWEEP=1<<4,
  TRIANGULAR_SWEEP=0
}CTRL_REG_D4;

typedef enum INT_EXT_INCR{
  EXT_INC=1<<5,
  INT_INC=0
}CTRL_REG_D5;

typedef enum INT_EXT_BURST{
  EXT_BUR=1<<6,
  INT_BUR=0
}CTRL_REG_D6;

typedef enum CW_BURST{
  CW=1<<7,
  BURST=0
}CTRL_REG_D7;

typedef enum MSBOUTEN{
  MSBOUT_EN=1<<8,
  MSBOUT_DIS=0
}CTRL_REG_D8;

typedef enum SINE_TRI{
  SINE=1<<9,
  TRI=0
}CTRL_REG_D9;

typedef enum DAC_ENABLE{
  DAC_EN=1<<10,
  DAC_DIS=0
}CTRL_REG_D10;

typedef enum TWICE_ONCE{
  TWICE=1<<11, // two consecutive writes will be performed
  ONCE=0 // one write will be performed
}CTRL_REG_D11;


const uint16_t ctrl_reg_val = CTRL_REG_MASK| // D15 - D12 address of the control register
                                TWICE| // Two write operations (two words) are required to load Fstart and Fdelta
                               DAC_EN| // DAC is enabled
                                 SINE| // Iout and Ioutb output sine waves
                           MSBOUT_DIS| // Disable the MSBOUT pin
                                   CW| // Output each frequency continuously (rather than bursts) to get a fixed-freq signal
                              EXT_BUR| // The frequency burst are triggered externally thorugh the CTRL pin 
                              EXT_INC| // The frequency increments are triggered externally through the CTRL pin
                            SAW_SWEEP; // Saw sweep mode 

const SPISettings settingsA(656000, MSBFIRST, SPI_MODE1); 

void setup() {
  
  // initialize the output pins
  digitalWrite(CTRL, HIGH);
  digitalWrite(FSYNC, HIGH);
  digitalWrite(SCLK, LOW);
  digitalWrite(SDATA, LOW);
  
  pinMode(CTRL, OUTPUT);
  pinMode(FSYNC, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(SDATA, OUTPUT);
}

void loop() {
  configAD5930();
  
  /* toggle CTRL pin to reset the internal state machine and begin outputing the sine wave
     If frequency increment is needed in your application, change the frequency increment and 
     the number of increments to be non-zero. Then use this function to triger the increment */
  toggleCtrlPin();

  /* do nothing in the while loop*/
  while(1){
    delay(1000);
  }
}

void setStartFreq(uint32_t freq){
  if(freq > EXTERN_CLOCK_FREQ/2) return; // Exceed max correct frequency
  uint32_t scaledVal = (freq * 1.0 / EXTERN_CLOCK_FREQ) * 0xffffff;
  uint16_t LSB_MASK = 0xc000;
  uint16_t MSB_MASK = 0xd000;
  uint16_t lsbs = scaledVal & 0xfff;
  uint16_t msbs = (scaledVal >> 12) & 0xfff;

  spiWriteWord(LSB_MASK | lsbs);
  spiWriteWord(MSB_MASK | msbs);
}

void setDeltaFreq(long freq){
  if(freq > EXTERN_CLOCK_FREQ/2 || freq < -EXTERN_CLOCK_FREQ/2) return; // Exceed max increcement frequency

  int sign_bit = 0;
  if(freq < 0) sign_bit = 1;
  freq = (freq>0 ? freq : -freq);
  uint32_t scaledVal = (freq * 1.0 / EXTERN_CLOCK_FREQ) * 0xffffff;
  
  uint16_t LSB_MASK = 0x2000;
  uint16_t MSB_MASK = 0x3000;
  
  uint16_t lsbs = scaledVal & 0xfff;
  uint16_t msbs = (scaledVal >> 12) & 0x7ff; 

  msbs = msbs | (sign_bit << 23); // Set the sign bit
  
  spiWriteWord(LSB_MASK | lsbs);
  spiWriteWord(MSB_MASK | msbs);
}

void setNumIncr(uint16_t num){
  if(num > 0xfff ) return; // Exceed max number of increcement 
  uint16_t N_MASK = 0x1000;
  spiWriteWord(N_MASK | (num & 0xfff));
}

void spiWriteWord(uint16_t val) {
  // Take the SS pin low to select the chip:
  digitalWrite(FSYNC,LOW);
  delay(1);
  
  // Send in the address and value via SPI:
  SPI.transfer((val >> 8) & 0xff);
  SPI.transfer(val & 0xff);
  
  delay(1);
  // Take the SS pin high to de-select the chip:
  digitalWrite(FSYNC,HIGH); 
}

void configAD5930(){
  SPI.begin();
  SPI.beginTransaction(settingsA);

  setStartFreq(100000); // Set start freqency 
  setDeltaFreq(0); // Set frequency increment
  setNumIncr(0); // Set number of increments

  /* not used in this code, check the datasheet carefully for your application */
  spiWriteWord(0x6000); // Set increment interval
  spiWriteWord(0x8000); // Set burst interval 
  
  spiWriteWord(ctrl_reg_val); //Set Control Reg (configuring control register has to be the last)
  SPI.endTransaction();
}

void toggleCtrlPin(){
    digitalWrite(CTRL, HIGH);
    delay(1);
    digitalWrite(CTRL, LOW);
}

