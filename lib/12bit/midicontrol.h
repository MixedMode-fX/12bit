#ifndef MIDICONTROL_H
#define MIDICONTROL_H

#include <USBHost_t36.h>
#include "midicc.h"
#include "12bitconfig.h"

USBHost usbHost = USBHost();
USBHub usbHub = USBHub(usbHost);
MIDIDevice usbMIDI = MIDIDevice(usbHost);


void handleCC(byte channel, byte control, byte value);

void midiBegin(){
    usbMIDI.begin();
    usbMIDI.setHandleControlChange(handleCC);
}

void midi(){
    usbHost.Task();
    usbMIDI.read();
}

#endif