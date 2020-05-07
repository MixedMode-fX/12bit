#include <Arduino.h>
#include <mcpcodec.h>

// Chip Select pins
#define CS_DAC 8
#define CS_ADC 9

#define N_CHANNELS 2
#define SAMPLE_PERIOD 20

IntervalTimer audioTimer;
void codec();

void setup(){
    spiBegin();
    pinMode(CS_DAC, OUTPUT);    pinMode(CS_ADC, OUTPUT);
    audioTimer.begin(codec, SAMPLE_PERIOD);
    audioTimer.priority(0);
}

void loop(){}

void codec(){
    for(uint8_t i=0; i<N_CHANNELS; i++){
        dac(adc(i, CS_ADC), i, CS_DAC);
    }
}