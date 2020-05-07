#include <Arduino.h>
#include <mcpcodec.h>

// Chip Select pins
#define CS_DAC 8
#define CS_ADC 9

#define N_CHANNELS 2

void setup(){
    spiBegin();
}

void loop(){
    for(uint8_t i=0; i<N_CHANNELS; i++){
        dac(adc(i, CS_ADC), i, CS_DAC);
    }
}