#ifndef FX_H
#define FX_H

#include <Arduino.h>

int16_t crush(int16_t value, uint8_t bit_reduction){ // reduced bit depth and adjust output volume
    return ((value >> bit_reduction) << bit_reduction);
}


#endif