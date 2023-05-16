#ifndef _12BITCONFIG_H
#define _12BITCONFIG_H

// chip select pins
#define CS_DAC 8
#define CS_ADC 9

// configuration for our system
#define N_CHANNELS 2
#define BIT_DEPTH 12
#define FULL_SCALE 4095
#define HALF_SCALE 2048
#define SCALE_CTRL_SHIFT 1

// sample rate control
// sample_rate [Hz] = 1 / ( sample_period * 10^-6 [s] )
// period is set in microseconds
#define DEFAULT_SAMPLE_PERIOD 120     // sample period
#define MIN_SAMPLE_PERIOD DEFAULT_SAMPLE_PERIOD/4                        // = 43478 Hz - close enough... we're doing lofi here
#define MAX_SAMPLE_PERIOD DEFAULT_SAMPLE_PERIOD*4       // = 5434 Hz - que viva aliasing


#define DEFAULT_SAMPLE_PERIOD_DAC 30     // sample period
#define MIN_SAMPLE_PERIOD_DAC DEFAULT_SAMPLE_PERIOD_DAC                        // = 43478 Hz - close enough... we're doing lofi here
#define MAX_SAMPLE_PERIOD_DAC DEFAULT_SAMPLE_PERIOD_DAC*12       // = 5434 Hz - que viva aliasing


// controls refresh rate
#define CONTROLS_PERIOD 150

// Volume
#define MIN_VOLUME 0
#define MAX_VOLUME 1.0

// LPF
#define MIN_LPF_CUTOFF 1
#define MAX_LPF_CUTOFF 255
#define DEFAULT_LPF_CUTOFF MAX_LPF_CUTOFF

// DELAY
#define DELAY_BUFFER_SIZE 16384 // 61000 is cool
#define MIN_DELAY_TIME 0

// TAPE RECORDER
#define TAPE_LENGTH 61000

#endif