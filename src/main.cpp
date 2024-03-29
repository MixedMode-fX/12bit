#include <Arduino.h>
#include <mcpcodec.h>
#include <midicontrol.h>
#include "12bitconfig.h"
#include "utils.h"
#include <fx.h>
#include <tapemachine.h>


// FX controls
uint8_t bit_reduction = 0;
uint16_t bit_mask = 0;
uint8_t bit_mask_enable = 1;
uint8_t gain = 0xFF;
uint8_t volume = 0xFF;

// Delay

uint16_t delay_time = DELAY_BUFFER_SIZE/2;      // in samples
uint16_t target_delay_time = DELAY_BUFFER_SIZE/2;

// Audio stream
uint16_t sample_period_dac = MIN_SAMPLE_PERIOD_DAC;
uint16_t sample_period_adc = MIN_SAMPLE_PERIOD;
int16_t input[N_CHANNELS], output[N_CHANNELS];

// FX
LPF input_lpf[N_CHANNELS] = LPF(DEFAULT_LPF_CUTOFF);
bool input_lpf_enable = 1;

TapeDelay<16> tape_delay(TAPE_LENGTH-1);
int16_t delay_output[N_CHANNELS];


// Timer for updating controls
IntervalTimer controlsTimer;
void controlsFilter();


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
    tape_delay.spin();

    // Input & playback
    for(uint8_t i=0; i<N_CHANNELS; i++){
        input[i] = adcDCOffset(i, CS_ADC);                          // read ADC and remove DC offset
        input[i] = crush(input[i], bit_reduction, bit_mask);        // reduce bit depth & apply mask
        if(input_lpf_enable) input[i] = input_lpf[i].apply(input[i]);  // low pass filter
        input[i] = scale8(input[i], gain);                          // input gain control
        tape_delay.rec(i, input[i]);                                // record to the tape
    }
}

void audioOut(){
    // Output mixer
    for(uint8_t i=0; i<N_CHANNELS; i++){
        output[i] = tape_delay.out(i);
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


            case CC_INPUT_FILTER_ENABLE:
                input_lpf_enable = value < 64;
                break;

            case CC_CUTOFF:
                input_lpf[0].setGain(MIDIMAP(value, MIN_LPF_CUTOFF, MAX_LPF_CUTOFF));
                input_lpf[1].setGain(MIDIMAP(value, MIN_LPF_CUTOFF, MAX_LPF_CUTOFF));
                break;

            case CC_VOLUME:
                volume = value << 1;
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
                tape_delay.handleCC(channel, control, value);
                break;

        }
    }
}


void controlsFilter(){
    // slowly adjust delay time towards target
    tape_delay.smoothDelay();
}