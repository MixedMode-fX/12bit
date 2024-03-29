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
        void setDelay(uint16_t d){ 
            delay_time = d; 
            delay_time %= record->getLength();
        }
        void setReverse(bool r){ reverse = r; }
        void setLevel(uint8_t l){ level = l; }

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

        int16_t playPreFader(uint8_t channel){
            return record->readTape(channel, (uint16_t)position);
        }


        int16_t play(uint8_t channel){
            return scale8(record->readTape(channel, (uint16_t)position), level);
        }


    private:
        RecordHead *record;
        uint16_t delay_time;
        int32_t position;
        bool reverse;
        uint8_t level = 0;

};


template<int taps>
class TapeDelay {

    public:
        TapeDelay(uint16_t d){
            for(uint8_t i=0; i<taps; i++){
                repro[i].setRecorder(&record);
                repro[i].setDelay(d);
            }
            repro[0].setLevel(0xFF);
            target_delay_time = d;
            delay_time = d;
        }

        void setDelay(uint16_t d){ 
            for(uint8_t i=0; i<taps; i++){
                repro[i].setDelay(d + i * head_spacing);
            }
        }
        void setDelayTarget(uint16_t dt){ target_delay_time = dt; }
        void setHeadSpacing(uint16_t s){ target_head_spacing = s; }
        void setHeadLevel(uint8_t head, uint8_t l){ repro[head].setLevel(l); }
        void setFeedback(uint8_t fb){ feedback_level = fb; }
        void setReverse(bool r){ 
            for(uint8_t i=0; i<taps; i++){
                repro[i].setReverse(r); 
            }
        }
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
            for(uint8_t i=0; i<taps; i++){
                repro[i].spin();
            }
        }

        void rec(uint8_t channel, int16_t input){
            input_signal[channel] = input;
            
            uint8_t fb_c = ping_pong ? (channel+1)%N_CHANNELS : channel;

            feedback[channel] = 0;
            for(uint8_t i=0; i<taps; i++){
                feedback[channel] += scale8(repro[i].play(fb_c), feedback_level);
            }
            
            uint16_t record_signal = input + feedback[channel];
            if (filter) record_signal = lpf[channel].apply(record_signal);
            record.rec(channel, record_signal);

            repro_signal[channel] = 0;
            for(uint8_t i=0; i<taps; i++){
                repro_signal[channel] += repro[i].play((fb_c + i * ping_pong) % N_CHANNELS);
            }
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

            if (target_head_spacing > head_spacing) {
                head_spacing += 1;
                setDelay(delay_time);
            }
            if (target_head_spacing < head_spacing) {
                head_spacing -= 1;
                setDelay(delay_time);
            }
        }

        void handleCC(byte channel, byte control, byte value){
        
            switch(control){
                case CC_DELAY_TIME:
                    setDelayTarget(MIDIMAP(value, MIN_DELAY_TIME, DELAY_BUFFER_SIZE-1));
                    break;
                case CC_DELAY_FEEDBACK:
                    setFeedback(value << SCALE_CTRL_SHIFT);
                    break;
                case CC_DELAY_CUTOFF:
                    setLPFCutoff(MIDIMAP(value, MIN_LPF_CUTOFF, MAX_LPF_CUTOFF));
                    break;
                case CC_DELAY_HEADSPACE:
                    setHeadSpacing(MIDIMAP(value, 0, DELAY_BUFFER_SIZE/2));
                    break;
                case CC_INPUT_MIX:
                    setInputMix(value << SCALE_CTRL_SHIFT);
                    break;
                case CC_DELAY_MIX:
                    setDelayMix(value << SCALE_CTRL_SHIFT);
                    break;
                case CC_DELAY_REVERSE:
                    setReverse(value > 64);
                    break;
                case CC_DELAY_PING_PONG:
                    setPingPong(value > 64);
                    break;
                case CC_DELAY_FILTER_ENABLE:
                    setLPF(value < 64);
                    break;

                case CC_DELAY_HEAD0:
                case CC_DELAY_HEAD1:
                case CC_DELAY_HEAD2:
                case CC_DELAY_HEAD3:
                case CC_DELAY_HEAD4:
                case CC_DELAY_HEAD5:
                case CC_DELAY_HEAD6:
                case CC_DELAY_HEAD7:
                case CC_DELAY_HEAD8:
                case CC_DELAY_HEAD9:
                case CC_DELAY_HEAD10:
                case CC_DELAY_HEAD11:
                case CC_DELAY_HEAD12:
                case CC_DELAY_HEAD13:
                case CC_DELAY_HEAD14:
                case CC_DELAY_HEAD15:
                    setHeadLevel(control - CC_DELAY_HEAD0, value);
                    break;
            }
        }

    private:
        // hardware
        RecordHead record;
        PlaybackHead repro[taps];
        LPF lpf[N_CHANNELS] = {LPF(DEFAULT_LPF_CUTOFF), LPF(DEFAULT_LPF_CUTOFF)};

        // signals
        int16_t input_signal[N_CHANNELS];
        int16_t repro_signal[N_CHANNELS];
        int16_t feedback[N_CHANNELS];
        int16_t mixer_output[N_CHANNELS];

        // parameters        
        bool ping_pong = 0;
        bool filter = 1;
        uint8_t feedback_level = 0;
        uint8_t input_mix = 127;
        uint8_t delay_mix = 127;
        uint16_t target_delay_time, delay_time;
        uint16_t target_head_spacing, head_spacing = 5000;


};

#endif