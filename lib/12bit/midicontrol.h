#ifndef MIDICONTROL_H
#define MIDICONTROL_H

#include <USBHost_t36.h>
#include "midicc.h"
#include "12bitconfig.h"

USBHost usbHost = USBHost();
USBHub usbHub = USBHub(usbHost);
MIDIDevice usbMIDIHost = MIDIDevice(usbHost);


void handleCC(byte channel, byte control, byte value);

// start both USB MIDI and USB HOST
void midiBegin(){
    usbMIDIHost.begin();
    usbMIDIHost.setHandleControlChange(handleCC);
    usbMIDI.setHandleControlChange(handleCC);
}

void midi(){
    usbHost.Task();
    usbMIDIHost.read();
    usbMIDI.read();
}

#endif