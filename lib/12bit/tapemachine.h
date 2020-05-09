#ifndef RECORDER_H
#define RECORDER_H

#include <Arduino.h>
#include "12bitconfig.h"
#include "fx.h"

class Record{

    public:
        Record(){ loop_length = TAPE_LENGTH; }

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

        int16_t readTape(uint8_t channel, uint16_t position){ return tape[channel][position]; }

        uint16_t getPosition(){ return position; }
        uint16_t getLength(){ return loop_length; }
        void setLength(uint16_t length){ loop_length = length; }

    private:
        uint16_t loop_length;
        int16_t tape[N_CHANNELS][TAPE_LENGTH];
        uint16_t position;
};


class Repro{

    public:
        Repro(){ delay_time = 0; }
        void setRecorder(Record *r){
            record = r;
        }
        void setDelay(uint16_t delay){ delay_time = delay; }

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
            return record->readTape(channel, position);
        }


    private:
        Record *record;
        uint16_t delay_time;
        uint16_t position;

        bool reverse;

};



class TapeDelay {

    public:
        TapeDelay(uint16_t delay){
            repro.setRecorder(&record);
            repro.setDelay(delay);
        }

        void setDelay(uint16_t delay){ repro.setDelay(delay); }
        void setFeedback(uint8_t fb){ feedback_level = fb; }

        void spin(){
            record.spin();
            repro.spin();
        }

        void rec(uint8_t channel, int16_t input){
            int16_t feedback = scale8(repro.play(channel), feedback_level);
            record.rec(channel, input + feedback);
        }

        void hold(){
            for(uint8_t i=0; i<N_CHANNELS; i++){
                sample_hold[i] = repro.play(i);
            }
        }

        int16_t out(uint8_t channel){
            return sample_hold[channel];
        }

    private:
        Record record;
        Repro repro;
        uint8_t feedback_level = 0;
        int16_t sample_hold[N_CHANNELS];

};

#endif