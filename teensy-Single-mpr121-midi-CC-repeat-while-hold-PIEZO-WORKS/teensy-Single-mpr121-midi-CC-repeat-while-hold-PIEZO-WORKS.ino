/*********************************************************
This is a library for the MPR121 12-channel Capacitive touch sensor

Designed specifically to work with the MPR121 Breakout in the Adafruit shop 
  ----> https://www.adafruit.com/products/

These sensors use I2C communicate, at least 2 pins are required 
to interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.  
BSD license, all text above must be included in any redistribution
**********************************************************/

//sketch in progress.  trying to read i2c from mpr121 and send midi message from teensy in response to which pin is touched. 
//important links: teensy wire library: https://www.pjrc.com/teensy/td_libs_Wire.html
// teensy midi library: https://www.pjrc.com/teensy/td_midi.html

//Drum stuff
int state[] = {0,0,0,0,0,0}; // 0=idle, 1=looking for peak, 2=ignore aftershocks
int peak[] = {0,0,0,0,0,0};    // remember the highest reading
elapsedMillis msec[] = {0,0,0,0,0,0}; // timer to end states 1 and 2
const int numDrumPins = 6;
const int drumNotes[] = {38,39,40,41,42,43};
const int drumPins[] = {A0, A1, A2, A7, A8, A9};
const int channel = 1;
const int thresholdMin = 12;  // minimum reading, avoid "noise"
const int aftershockMillis = 60; // time of aftershocks & vibration


#include <Wire.h>
#include "Adafruit_MPR121.h"
#include <Ticker.h>
#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif
//This is the duration between sends of midi signal in Milliseconds.
#define TRIGGER_DURATION 100


void triggerLoop(); // hoisted, defined below.
// Ticker Library sets up timers and a function to call when the timer elapses
           //(functioncalled, timertime, number to repeat(0is forever, RESOLUTION)
Ticker repeatTimer(triggerLoop, TRIGGER_DURATION, 0, MILLIS);
// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();
// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;
const uint8_t numElectrodes = 12; //added by drc
const uint8_t notes[numElectrodes] = {36, 38, 40, 43, 45, 47, 48, 50, 52, 55, 57, 60}; //added by drc
const uint8_t controlNumA[] = {88,88,87,87,13,13,5,5,0,0,3,3}; //Change to #'s stefan is using. 88, 87, 13, 5, 0, 3, 50 is mode shift
const uint8_t controlValA[] = {127,0,127,0,127,0,127,0,127,0,127,0}; //Change to #'s stefan is using
uint8_t ElectrodeTouched[numElectrodes] = {0,0,0,0,0,0,0,0,0,0,0,0};


void setup() {
  
  
  Wire.begin(0x5A); //added by drc
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); //added by drc
  Wire.setSDA(18); //use a 4.7k pullup resistor //added by drc
  Wire.setSCL(19); //use a 4.7k pullup resistor //added by drc
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1); 
    }
  Serial.println("MPR121 found!");
  repeatTimer.start();

//from drum code.  not sure if its even necessary
  while (!Serial && millis() < 2500) /* wait for serial monitor */ ;
  Serial.println("Piezo Peak Capture");  
}

void loop() {

//drum stuff
while (usbMIDI.read()) { } // ignore incoming messages??
  for (uint8_t x=0; x<numDrumPins; x++) { //changed i<0 to i<numElectrodes
    int value = analogRead(drumPins[x]);

    if (state[x] == 0) {
      // IDLE state: if any reading is above a threshold, begin peak
      if (value > thresholdMin) {
        //Serial.println("begin state 1");
        state[x] = 1;
        peak[x] = value;
        msec[x] = 0;
      }
    } else if (state[x] == 1) {
      // Peak Tracking state: for 10 ms, capture largest reading
      if (value > peak[x]) {
        peak[x] = value;
      }
      if (msec[x] >= 10) {
        Serial.print("peak = ");
        Serial.println(peak[x]);
        //Serial.println("begin state 2");
        int velocity = map(peak[x], thresholdMin, 1023, 1, 127);
        usbMIDI.sendNoteOn(drumNotes[x], velocity, channel);
        state[x] = 2;
        msec[x] = 0;
      }
    } else {
      // Ignore Aftershock state: wait for things to be quiet again
      if (value > thresholdMin) {
        msec[x] = 0; // keep resetting timer if above threshold
      } else if (msec[x] > 30) {
        //Serial.println("begin state 0");
        usbMIDI.sendNoteOff(drumNotes[x], 0, channel);
        state[x] = 0; // go back to idle after 30 ms below threshold
      }
    }
  } 


  // Get the currently touched pads
  currtouched = cap.touched();
  repeatTimer.update();
  // timer is runnig as long as this is getting called.
  checkElectrodes();
}

void checkElectrodes(){

  for (uint8_t i=0; i<numElectrodes; i++) { //changed i<0 to i<numElectrodes
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {


      digitalWrite (LED_BUILTIN, HIGH); //added by drc

      Serial.print(i); Serial.println(" touched");
      // set the array value to 1 on touch
      ElectrodeTouched[i] = 1;
    }
    // if it *was* touched and now *isnt*, alert! 
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {

      digitalWrite (LED_BUILTIN, LOW);
      Serial.print(i); Serial.println(" released");
      // set it back to 0 on release
      ElectrodeTouched[i] = 0;   
    }
  }

  // reset our state
  lasttouched = currtouched;
}

void triggerLoop(){
  // this fires on a timer and anything pressed triggers midi
  for (uint8_t i=0; i<numElectrodes; i++) { 
    if (ElectrodeTouched[i]) {
      triggerMidi(i);
    }
  }
  //Serial.print(ElectrodeTouched[0]);
  //Serial.print("trigger Loop");

}

void triggerMidi(int i){
   usbMIDI.sendControlChange(controlNumA[i], controlValA[i], channel); //(control#, controlval, channel)
   Serial.print("triggered midi on: ");
   Serial.println(i);
}

