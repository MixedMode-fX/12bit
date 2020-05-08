#include <Arduino.h>
#include <mcpcodec.h>
#include <midicontrol.h>
#include "12bitconfig.h"
#include "utils.h"
#include <fx.h>


// FX controls
uint8_t bit_reduction = 0;
uint8_t volume = 127;
float lpf_cutoff = DEFAULT_LPF_CUTOFF;

// Delay
int16_t buffer[N_CHANNELS][DELAY_BUFFER_SIZE];    // this is our tape loop
int16_t play_head[N_CHANNELS];              // current value played back from the tape
uint16_t rec_index;                         // position of the record head
int32_t play_index;                         // position of the playback head
uint16_t delay_time = DELAY_BUFFER_SIZE/2;  // in samples
uint16_t target_delay_time = DELAY_BUFFER_SIZE/2;  // in samples
uint8_t delay_mix = 127;
uint8_t delay_feedback, delay_reverse, delay_ping_pong;

IntervalTimer controlsTimer;                // delay time smoothing
void controlsFilter();

// Audio stream
uint16_t sample_period = MIN_SAMPLE_PERIOD;
int16_t input[N_CHANNELS];
int16_t output[N_CHANNELS], prev_output[N_CHANNELS];


void setup(){
    codecBegin();
    midiBegin();

    controlsTimer.begin(controlsFilter, CONTROLS_PERIOD);
    controlsTimer.priority(10);
}

void loop(){
    midi();
}

void audio(){

    // delay write and read index
    rec_index = (rec_index + 1) % DELAY_BUFFER_SIZE;
    if(!delay_reverse){
        play_index =  (rec_index - delay_time);
    } else {
        play_index = DELAY_BUFFER_SIZE - (rec_index - delay_time);
    }
    if (play_index < 0) play_index += DELAY_BUFFER_SIZE;
    play_index %= DELAY_BUFFER_SIZE;


    int16_t delay_signal[N_CHANNELS];
    for(uint8_t i=0; i<N_CHANNELS; i++){
        prev_output[i] = output[i];
        input[i] = adcDCOffset(i, CS_ADC);                          // read ADC and remove DC offset
        output[i] = crush(input[i], bit_reduction);                 // reduce bit depth
        output[i] = lpf(output[i], prev_output[i], lpf_cutoff);     // low pass filter
        output[i] = scale(output[i], volume);                       // volume control

        play_head[i] = buffer[delay_ping_pong ? (i+1)%N_CHANNELS : i][(uint16_t)play_index]; // when in ping pong, get the delay from the next channel
        play_head[i] = scale(play_head[i], delay_feedback); 
        buffer[i][rec_index] = output[i] + play_head[i];            // record our signal to our buffer + add a fraction of what's on the play head

        delay_signal[i] = crossfade(play_head[i], output[i], delay_mix);    // mix the input + delay
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
                target_delay_time = MIDIMAP(value, MIN_DELAY_TIME, DELAY_BUFFER_SIZE-1);
                break;
            case CC_DELAY_FEEDBACK:
                delay_feedback = value << 1;
                break;
            case CC_DELAY_MIX:
                delay_mix = value << 1;
                break;
            case CC_DELAY_REVERSE:
                delay_reverse = value > 64;
                break;
            case CC_DELAY_PING_PONG:
                delay_ping_pong = value > 64;
                break;

        }
    }
}


void controlsFilter(){
    // slowly adjust delay time towards target
    if (target_delay_time > delay_time) {
        delay_time += 1;
    }
    if (target_delay_time < delay_time) {
        delay_time -= 1;
    }
}