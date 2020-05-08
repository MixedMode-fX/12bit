#ifndef FX_H
#define FX_H

#include <Arduino.h>

/*
 *
 * Volume scaling
 * Source: used for feedback scaling in BleepLab RadFi 1.5 
 * https://github.com/BleepLabs/Rad-Fi-Delay-1.5
 * 
 */
int16_t scale8(int16_t value, uint8_t amplitude){ 
    return (amplitude * value) >> 8; 
}

int16_t scale12(int16_t value, uint16_t amplitude){ 
    return (amplitude * value) >> BIT_DEPTH; 
}


int16_t crossfade(int16_t value_a, int16_t value_b, uint8_t mix_control){
    return scale8(value_a, mix_control) + scale8(value_b, 255 - mix_control);
}

/*
 *
 * Set least significant bits to 0
 * 
 */
int16_t crush(int16_t value, uint8_t bit_reduction){ // reduced bit depth and adjust output volume
    return ((value >> bit_reduction) << bit_reduction);
}


/*
 *
 * Simple Low Pass Filter
 * Source: Beam Myself Into The Future
 * https://beammyselfintothefuture.wordpress.com/2015/02/16/simple-c-code-for-resonant-lpf-hpf-filters-and-high-low-shelving-eqs/
 * 
 */
int16_t lpf(int16_t sample, int16_t prev_sample, double cutoff) {
    return cutoff * (sample - prev_sample) + prev_sample;
}


/*
 *
 * Simple soft clipper
 * Source: BleepLabs Bleep Drum 15
 * Github: 
 *
 */
int16_t soft_clip(int16_t sample){
    sample += HALF_SCALE;
    if (sample > FULL_SCALE) {
        sample -= (sample - FULL_SCALE) << 1;
    }
    if (sample < 0) {
        sample += sample * -2;
    }
    return sample - HALF_SCALE;
}


#endif