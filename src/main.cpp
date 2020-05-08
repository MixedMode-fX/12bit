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

// Delay
int16_t buffer[N_CHANNELS][DELAY_BUFFER_SIZE];    // this is our tape loop
int16_t play_head[N_CHANNELS];              // current value played back from the tape
uint16_t rec_index;                         // position of the record head
int32_t play_index;                         // position of the playback head
uint16_t delay_time = DELAY_BUFFER_SIZE/2;                        // in samples
uint8_t delay_mix = 127;

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

    // delay write and read index
    rec_index = (rec_index + 1) % DELAY_BUFFER_SIZE;
    play_index =  (rec_index - delay_time);
    if (play_index < 0) play_index += DELAY_BUFFER_SIZE;
    play_index %= DELAY_BUFFER_SIZE;


    int16_t delay_signal[N_CHANNELS];
    for(uint8_t i=0; i<N_CHANNELS; i++){
        prev_output[i] = output[i];
        input[i] = adcDCOffset(i, CS_ADC);                          // read ADC and remove DC offset
        output[i] = crush(input[i], bit_reduction);                 // reduce bit depth
        output[i] = lpf(output[i], prev_output[i], lpf_cutoff);     // low pass filter
        output[i] = scale(output[i], volume);                       // volume control

        buffer[i][rec_index] = output[i];                           // record our signal to our buffer
        play_head[i] = buffer[i][(uint16_t)play_index];
        
        delay_signal[i] = crossfade(output[i], play_head[i], delay_mix);
        delay_signal[i] = soft_clip(delay_signal[i]);               // soft clipper to avoid nasty distortion if the signal exceeds FULL_SCALE
        dacDCOffset(delay_signal[i], i, CS_DAC);                    // write to the dac and apply DC offset required
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
            case CC_DELAY_TIME:
                delay_time = MIDIMAP(value, MIN_DELAY_TIME, DELAY_BUFFER_SIZE);
                break;
            case CC_DELAY_MIX:
                delay_mix = value << 1;
                break;

        }
    }
}