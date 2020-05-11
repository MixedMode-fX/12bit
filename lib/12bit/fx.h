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
 * Low Pass Filter
 * Source: Beam Myself Into The Future
 * https://beammyselfintothefuture.wordpress.com/2015/02/16/simple-c-code-for-resonant-lpf-hpf-filters-and-high-low-shelving-eqs/
 *
 */

class LPF{

    /*
     * First order low pass filter
     * 
     * Source: Matlab implementation of Reverberation Algorithms
     * Authors: Fernando Beltran, José Ramón Beltrán Blázquez, Nicolas Holzem, Adrian Gogu
     * https://www.researchgate.net/publication/2460443_Matlab_Implementation_of_Reverberation_Algorithms
     * 
     * H(z) = (1 - g) / (1 - g * z^-1)
     *
     * fc = cutoff frequency
     * fs = sampling frequency
     * tau = 2*pi*fc/fs
     * g = 2 - cos(tau) - sqrt( (cos(tau)) - 2 ) ^ 2 - 1 )
     * g was previously called 'cutoff'
     * 
     */

    public:
        LPF(double g){ _g = g; };
        int16_t apply(int16_t sample){
            _prev_sample += _g * (sample - _prev_sample);
            return _prev_sample;
        }

        void setGain(double g){
            _g = g;
        }

    private:
        double _g;
        int16_t _prev_sample;
};


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