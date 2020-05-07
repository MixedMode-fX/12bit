#include <Arduino.h>
#include <mcpcodec.h>

// Chip Select pins
#define CS_DAC 8
#define CS_ADC 9

#define N_CHANNELS 2
#define SAMPLE_PERIOD 20

IntervalTimer audioTimer;
void audio();

void setup(){
    codecBegin();
    pinMode(CS_DAC, OUTPUT);    pinMode(CS_ADC, OUTPUT);
    audioTimer.begin(audio, SAMPLE_PERIOD);
    audioTimer.priority(0);
}

void loop(){}

void audio(){
    for(uint8_t i=0; i<N_CHANNELS; i++){
        dac(adc(i, CS_ADC), i, CS_DAC);
    }
}