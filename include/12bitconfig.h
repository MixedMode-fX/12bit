#ifndef _12BITCONFIG_H
#define _12BITCONFIG_H

// chip select pins
#define CS_DAC 8
#define CS_ADC 9

// configuration for our system
#define N_CHANNELS 2
#define BIT_DEPTH 12
#define FULL_SCALE 4095

// sample rate control
// sample_rate [Hz] = 1 / ( sample_period * 10^-6 [s] )
// period is set in microseconds
#define MIN_SAMPLE_PERIOD 23                        // = 43478 Hz - close enough... we're doing lofi here
#define MAX_SAMPLE_PERIOD MIN_SAMPLE_PERIOD*8       // = 5434 Hz - que viva aliasing
#define DEFAULT_SAMPLE_PERIOD MIN_SAMPLE_PERIOD     // sample period



#endif