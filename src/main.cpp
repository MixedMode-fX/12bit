#include <Arduino.h>
#include <mcpcodec.h>
#include <midicontrol.h>
#include "12bitconfig.h"
#include "utils.h"
#include <fx.h>


// FX controls
uint8_t bit_reduction = 0;
uint8_t gain = 0xFF;
uint8_t volume = 0xFF;


float lpf_cutoff = DEFAULT_LPF_CUTOFF;

// Delay
int16_t tape[N_CHANNELS][DELAY_BUFFER_SIZE];    // this is our tape loop
int16_t play_head[N_CHANNELS];                  // current value played back from the tape
int16_t feedback[N_CHANNELS];                   // scaled down version of the play_head signal, to be recorded back to tape 
uint16_t rec_index;                             // position of the record head
int32_t play_index;                             // position of the playback head
uint16_t delay_time = DELAY_BUFFER_SIZE/2;      // in samples
uint16_t target_delay_time = DELAY_BUFFER_SIZE/2;
uint8_t input_mix = 127;
uint8_t delay_mix = 127;
uint8_t delay_feedback, delay_reverse, delay_ping_pong;

// Audio stream
uint16_t sample_period = MIN_SAMPLE_PERIOD;
int16_t input[N_CHANNELS], prev_input[N_CHANNELS], output[N_CHANNELS];

// Timer for updating controls
IntervalTimer controlsTimer;
void controlsFilter();


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


    // Input & playback
    for(uint8_t i=0; i<N_CHANNELS; i++){
        input[i] = adcDCOffset(i, CS_ADC);                           // read ADC and remove DC offset
        input[i] = crush(input[i], bit_reduction);                   // reduce bit depth
        input[i] = lpf(input[i], prev_input[i], lpf_cutoff);         // low pass filter
        prev_input[i] = input[i];                                    // this is the z^-1 delay for the filter
        input[i] = scale8(input[i], gain);                           // input gain control

        play_head[i] = tape[i][(uint16_t)play_index];                // read back the tape
    }

    // Output mixer
    for(uint8_t i=0; i<N_CHANNELS; i++){
        uint8_t fb_c = delay_ping_pong ? (i+1)%N_CHANNELS : i;       // feedback channel
        feedback[i] = scale8(play_head[fb_c], delay_feedback);       // feedback is not taken from either the same or the other channel when set to ping pong
        tape[i][rec_index] = input[i] + feedback[i];                 // record the signal to tape + add a fraction of what's on the play head

        output[i]  = scale8(input[i], input_mix); 
        output[i] += scale8(play_head[i], delay_mix);
        output[i]  = scale8(output[i], volume);
        output[i]  = soft_clip(output[i]);                           // soft clipper to avoid nasty distortion if the signal exceeds FULL_SCALE
        dacDCOffset(output[i], i, CS_DAC);                           // write to the dac and apply DC offset required
    }
}

void handleCC(byte channel, byte control, byte value){
    if (channel == 1){
        switch(control){
            case CC_GAIN:
                gain = value << SCALE_CTRL_SHIFT;
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
                delay_feedback = value << SCALE_CTRL_SHIFT;
                break;
            case CC_INPUT_MIX:
                input_mix = value << SCALE_CTRL_SHIFT;
                break;
            case CC_DELAY_MIX:
                delay_mix = value << 1;
                break;
            case CC_VOLUME:
                volume = value << 1;
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