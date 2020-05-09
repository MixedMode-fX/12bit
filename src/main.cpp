#include <Arduino.h>
#include <mcpcodec.h>
#include <midicontrol.h>
#include "12bitconfig.h"
#include "utils.h"
#include <fx.h>


// FX controls
uint16_t sample_period = MIN_SAMPLE_PERIOD;
uint8_t bit_reduction = 0;
uint8_t volume = 127;
float lpf_cutoff = DEFAULT_LPF_CUTOFF;

// Audio stream
int16_t input[N_CHANNELS];
int16_t output[N_CHANNELS], prev_output[N_CHANNELS];

void setup(){
    codecBegin();
    midiBegin();
}

void loop(){
    midi();
}

void audio(){
    for(uint8_t i=0; i<N_CHANNELS; i++){
        input[i] = adcDCOffset(i, CS_ADC);                          // read ADC and remove DC offset
        output[i] = crush(input[i], bit_reduction);                 // reduce bit depth
        output[i] = lpf(output[i], prev_output[i], lpf_cutoff);     // low pass filter
        prev_output[i] = output[i];
        output[i] = scale(output[i], volume);                       // volume control
        output[i] = soft_clip(output[i]);                           // soft clipper to avoid nasty distortion if the signal exceeds FULL_SCALE
        dacDCOffset(output[i], i, CS_DAC);                          // write to the dac and apply DC offset required
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
            case CC_CUTOFF:
                lpf_cutoff = MIDIMAPF(value, MIN_LPF_CUTOFF, MAX_LPF_CUTOFF);
                break;
        }
    }
}