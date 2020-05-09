#include <Arduino.h>
#include <mcpcodec.h>
#include <midicontrol.h>
#include "12bitconfig.h"
#include "utils.h"
#include <fx.h>


uint16_t sample_period = MIN_SAMPLE_PERIOD;
uint8_t bit_reduction = 0;
uint8_t volume = 255;

void setup(){
    codecBegin();
    midiBegin();
}

void loop(){
    midi();
}

void audio(){
    int16_t input, output;
    for(uint8_t i=0; i<N_CHANNELS; i++){
        input = adc(i, CS_ADC);
        input -= HALF_SCALE;                                    // remove DC offset
        output = volume * crush(input, bit_reduction) >> 8;     // reduce bit depth & scale down
        output += HALF_SCALE;                                   // re-introduce DC offset
        dac(output, i, CS_DAC);
    }
}

void handleCC(byte channel, byte control, byte value){
    if (channel == 1){
        switch(control){
            case CC_VOLUME:
                volume = value << 1;
                break;
            case CC_BIT_REDUCTION:
                bit_reduction = MIDIMAP(value, 0, BIT_DEPTH-1);
                break;
            case CC_SAMPLE_RATE:
                sample_period = MIDIMAP(value, MIN_SAMPLE_PERIOD, MAX_SAMPLE_PERIOD);
                audioTimer.update(sample_period);
                break;
        }
    }
}