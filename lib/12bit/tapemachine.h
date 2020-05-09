#ifndef RECORDER_H
#define RECORDER_H

#include <Arduino.h>
#include <fx.h>
#include "12bitconfig.h"


class RecordHead{

    public:
        RecordHead(){ loop_length = TAPE_LENGTH; }

        void spin(){
            // spin the tape
            position = (position + 1) % loop_length;
        }

        void rec(uint8_t channel, int16_t signal){
            // erase and record
            tape[channel][position] = signal;
        }

        void sos(uint8_t channel, int16_t signal){
            // sound on sound
            // record without erasing
            tape[channel][position] += signal;
        }

        int16_t readTape(uint8_t channel, uint16_t p){ return tape[channel][p]; }
        
        uint16_t getPosition(){ return position; }
        uint16_t getLength(){ return loop_length; }
        void setLength(uint16_t length){ loop_length = length; }

    private:
        uint16_t loop_length;
        int16_t tape[N_CHANNELS][TAPE_LENGTH];
        uint16_t position;
};


class PlaybackHead{

    public:
        PlaybackHead(){ delay_time = 0; }
        void setRecorder(RecordHead *r){
            record = r;
        }
        void setDelay(uint16_t d){ delay_time = d; }
        void setReverse(bool r){ reverse = r; }

        void spin(){
            // spin the tape
            if(!reverse){
                position = (record->getPosition() - delay_time);
            } else {
                position = record->getLength() - (record->getPosition() - delay_time);
            }
            if (position < 0) position += record->getLength();
            position %= record->getLength();
        }

        int16_t play(uint8_t channel){
            return record->readTape(channel, (uint16_t)position);
        }


    private:
        RecordHead *record;
        uint16_t delay_time;
        int32_t position;
        bool reverse;

};



class TapeDelay {

    public:
        TapeDelay(uint16_t d){
            repro.setRecorder(&record);
            repro.setDelay(d);
            target_delay_time = d;
            delay_time = d;
        }

        void setDelay(uint16_t d){ repro.setDelay(d); }
        void setDelayTarget(uint16_t dt){ target_delay_time = dt; }
        void setFeedback(uint8_t fb){ feedback_level = fb; }
        void setReverse(bool r){ repro.setReverse(r); }
        void setPingPong(bool p){ ping_pong = p; }
        void setLPF(bool f){ filter = f; }
        void setLPFCutoff(float c){ 
            for(uint8_t i=0; i<N_CHANNELS; i++){
                lpf[i].setGain(c);
            }
        }
        void setInputMix(uint8_t m){ input_mix = m; }
        void setDelayMix(uint8_t m){ delay_mix = m; }

        void spin(){
            record.spin();
            repro.spin();
        }

        void rec(uint8_t channel, int16_t input){
            input_signal[channel] = input;
            uint8_t fb_c = ping_pong ? (channel+1)%N_CHANNELS : channel;
            feedback[channel] = scale8(repro.play(fb_c), feedback_level);
            if (filter) feedback[channel] = lpf[channel].apply(feedback[channel]);
            record.rec(channel, input + feedback[channel]);
            repro_signal[channel] = repro.play(fb_c);
        }

        int16_t out(uint8_t channel){
            int16_t output = scale8(input_signal[channel], input_mix); 
            output += scale8(repro_signal[channel], delay_mix);
            return output;
        }

        void smoothDelay(){
            if (target_delay_time > delay_time) {
                delay_time += 1;
                setDelay(delay_time);
            }
            if (target_delay_time < delay_time) {
                delay_time -= 1;
                setDelay(delay_time);
            }
        }

    private:
        // hardware
        RecordHead record;
        PlaybackHead repro;
        LPF lpf[N_CHANNELS] = {LPF(DEFAULT_LPF_CUTOFF), LPF(DEFAULT_LPF_CUTOFF)};

        // signals
        int16_t input_signal[N_CHANNELS];
        int16_t repro_signal[N_CHANNELS];
        int16_t feedback[N_CHANNELS];
        int16_t mixer_output[N_CHANNELS];

        // parameters        
        bool ping_pong = 0;
        bool filter = 0;
        uint8_t feedback_level = 0;
        uint8_t input_mix = 127;
        uint8_t delay_mix = 127;
        uint16_t target_delay_time, delay_time;


};

#endif