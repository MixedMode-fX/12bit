#include <Arduino.h>
#include <mcpcodec.h>
#include <midicontrol.h>
#include "12bitconfig.h"
#include "utils.h"
#include <fx.h>


// FX controls
uint8_t bit_reduction = 0;
uint16_t bit_mask = 0;
uint8_t bit_mask_enable = 1;
uint8_t gain = 0xFF;
uint8_t volume = 0xF0;

// Delay
int16_t buffer[N_CHANNELS][DELAY_BUFFER_SIZE];    // this is our tape loop
int16_t play_head[N_CHANNELS];              // current value played back from the tape
uint16_t rec_index;                         // position of the record head
int32_t play_index;                         // position of the playback head
uint16_t delay_time = DELAY_BUFFER_SIZE/2;  // in samples
uint16_t target_delay_time = DELAY_BUFFER_SIZE/2;  // in samples
uint8_t input_mix = 127;
uint8_t delay_mix = 127;
uint8_t delay_feedback, delay_reverse, delay_ping_pong;

IntervalTimer controlsTimer;                // delay time smoothing
void controlsFilter();

// Audio stream
uint16_t sample_period_dac = MIN_SAMPLE_PERIOD_DAC;
uint16_t sample_period_adc = MIN_SAMPLE_PERIOD;
int16_t input[N_CHANNELS], feedback[N_CHANNELS];
int16_t output[N_CHANNELS];

// FX
LPF input_lpf[N_CHANNELS] = LPF(DEFAULT_LPF_CUTOFF);

void setup(){
    codecBegin();
    midiBegin();

    audioTimerDAC.update(sample_period_dac);
    audioTimerADC.update(sample_period_adc);

    controlsTimer.begin(controlsFilter, CONTROLS_PERIOD);
    controlsTimer.priority(10);
}

void loop(){
    midi();
}

void audioIn(){

    // delay write and read index
    rec_index = (rec_index + 1) % DELAY_BUFFER_SIZE;
    if(!delay_reverse){
        play_index =  (rec_index - delay_time);
    } else {
        play_index = DELAY_BUFFER_SIZE - (rec_index - delay_time);
    }
    if (play_index < 0) play_index += DELAY_BUFFER_SIZE;
    play_index %= DELAY_BUFFER_SIZE;


    for(uint8_t i=0; i<N_CHANNELS; i++){
        input[i] = -adcDCOffset(i, CS_ADC);                          // read ADC and remove DC offset, input buffer is phase inverting
        input[i] = scale8(input[i], gain) << 2; 
        input[i] = crush(input[i], bit_reduction, bit_mask);                 // reduce bit depth
        input[i] = input_lpf[i].apply(input[i]);     // low pass filter


        uint8_t fb_c = delay_ping_pong ? (i+1)%N_CHANNELS : i; // feedback channel
        play_head[i] = buffer[i][(uint16_t)play_index]; // when in ping pong, get the delay from the next channel
        feedback[i] = scale8(play_head[fb_c], delay_feedback); 
        buffer[i][rec_index] = input[i] + feedback[i];            // record our signal to our buffer + add a fraction of what's on the play head
    }
}

void audioOut(){
    int16_t delay_signal[N_CHANNELS];
    for(uint8_t i=0; i<N_CHANNELS; i++){
        delay_signal[i]  = scale8(input[i], input_mix);
        delay_signal[i] += scale8(play_head[i], delay_mix);
        delay_signal[i]  = scale8(delay_signal[i], volume);
        delay_signal[i]  = soft_clip(delay_signal[i]);               // soft clipper to avoid nasty distortion if the signal exceeds FULL_SCALE

        dacDCOffset(delay_signal[i], i, CS_DAC);                    // write to the dac and apply DC offset required
    }
}

void handleCC(byte channel, byte control, byte value){
    if (channel == 1){
        switch(control){
            case CC_GAIN:
                gain = value << SCALE_CTRL_SHIFT;
                break;

            case CC_CUTOFF:
                input_lpf[0].setGain(MIDIMAPF(value, MIN_LPF_CUTOFF, MAX_LPF_CUTOFF));
                input_lpf[1].setGain(MIDIMAPF(value, MIN_LPF_CUTOFF, MAX_LPF_CUTOFF));
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

            case CC_BIT_REDUCTION:
                bit_reduction = MIDIMAP(value, 0, BIT_DEPTH-1);
                break;

            case CC_SAMPLE_PERIOD_DAC:
                sample_period_dac = MIDIMAP(value, MIN_SAMPLE_PERIOD_DAC, MAX_SAMPLE_PERIOD_DAC);
                audioTimerDAC.update(sample_period_dac);
                break;

            case CC_SAMPLE_PERIOD_ADC:
                sample_period_adc = MIDIMAP(value, MIN_SAMPLE_PERIOD, MAX_SAMPLE_PERIOD);
                audioTimerADC.update(sample_period_adc);
                break;

            case CC_BIT_B0:
            case CC_BIT_B1:
            case CC_BIT_B2:
            case CC_BIT_B3:
            case CC_BIT_B4:
            case CC_BIT_B5:
            case CC_BIT_B6:
            case CC_BIT_B7:

                if (value > 64){
                    bit_mask |= 1 << (control - CC_BIT_B0 + 4);
                } else {
                    bit_mask &= 0 << (control - CC_BIT_B0 + 4);
                }
                break;

            default:
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