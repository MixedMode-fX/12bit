#include <SPI.h>

// Simple library for MCP48x2 DAC & MCP3202 ADC


// SPI initialisation for ADC & DAC
void codecBegin(){
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
}

// Sample *channel* on ADC with 
int16_t adc(uint8_t channel, uint8_t cs_pin){
  uint8_t data_in = 0;
  uint16_t result = 0;
  digitalWrite(cs_pin, LOW);
  uint8_t data_out = 0b00000001;
  data_in = SPI.transfer(data_out);
  data_out = (channel == 0) ? 0b10100000 : 0b11100000;
  data_in = SPI.transfer(data_out);
  result = data_in & 0x0F;
  data_in = SPI.transfer(0x00);
  result = result << 8;
  result = result | data_in;
  digitalWrite(cs_pin, HIGH);
  return result;
}


// Output *value* on DAC *channel*
void dac(int16_t value, uint8_t channel, uint8_t cs_pin){
  uint16_t dac_out = (channel << 15) | (1 << 14) | (0 << 13) | (1 << 12) | ( value & 0xfff );
  digitalWrite(cs_pin, LOW);
  SPI.transfer(dac_out >> 8);
  SPI.transfer(dac_out & 255);
  digitalWrite(cs_pin, HIGH);
}
