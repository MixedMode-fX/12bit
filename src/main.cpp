#include <Arduino.h>
#include <mcpcodec.h>
#include <midicontrol.h>
#include <12bitconfig.h>

uint16_t sample_period = MIN_SAMPLE_PERIOD;

void setup(){
    codecBegin();
    midiBegin();
}

void loop(){
    midi();
}

void audio(){
    for(uint8_t i=0; i<N_CHANNELS; i++){
        dac(adc(i, CS_ADC), i, CS_DAC);
    }
}

void handleCC(byte channel, byte control, byte value){
    if (channel == 1){
        switch(control){
            case CC_SAMPLE_RATE:
                sample_period = map(value, 0, 127, MIN_SAMPLE_PERIOD, MAX_SAMPLE_PERIOD);
                audioTimer.update(sample_period);
                break;
        }
    }
}