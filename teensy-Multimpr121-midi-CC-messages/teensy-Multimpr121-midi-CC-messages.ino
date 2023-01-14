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

//important links: teensy wire library: https://www.pjrc.com/teensy/td_libs_Wire.html
// teensy midi library: https://www.pjrc.com/teensy/td_midi.html
// how to combine multiple MPR121's: https://forums.adafruit.com/viewtopic.php?p=764282#p764282 
change the type of midi message being sent on line 103 and beyond.
***change numElectrodes to cover 24 notes instead of 12 (will need to change variables in the for loops)
For using one MPR121, ADD (address pin of MPR121) pin is not connected to any other pin. Which means it is connected to the ground.
For using two or more (multiple) MPR121s,
ADD pin (of 1st MPR121) not connected to any other pin. This will have address 0x5A (Similar to that of single MPR121)
ADD pin (of 2nd MPR121) connected to Arduino 3.3V. This will have address 0x5B.
ADD pin (of 3rd MPR121) connected to SDA pin of MPR121 itself. This will have address 0x5C.
ADD pin (of 4th MPR121) connected to SCL pin of MPR121 itself. This will have address 0x5D.

Now join all 5V pins (of all the MPR121s) and connect it to 5V of Arduino. (3.3v with generic mprs)
Similarly, join all Ground pins (of all the MPR121s) to Arduino Ground, 
join all SDA pins (of all the MPR121s) to A4 of Arduino UNO (or SDA pin 20 of Arduino MEGA)
and join all SCL pins (of all the MPR121s) to A5 of Arduino UNO (or SCL 21 pin of Arduino MEGA). 
All MPR121s will have 4 pins connected to Arduino, except for the one using 0x0B address which will have ADD pin connected to 3.3V.
**********************************************************/

#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();
Adafruit_MPR121 cap2 = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

uint16_t lasttouched2 = 0;
uint16_t currtouched2 = 0;

const uint8_t numElectrodes = 24; //added by drc
//const uint8_t notes[numElectrodes] = {36, 38, 40, 43, 45, 47, 48, 50, 52, 55, 57, 60}; //added by drc
const uint8_t controlNumA[] = {1,2,3,4,5,6,7,8,9,10,11,12}; //Change to #'s stefan is using
const uint8_t controlNumB[] = {13,14,15,16,17,18,19,20,21,22,23,24}; //Change to #'s stefan is using

int channel = 1; //added by drc // channel 0 was returning channel 16 in midi monitor


void setup() {
  Wire.begin(0x5A); //added by drc
  Wire.begin(0x5B); //added by drc

  Serial.begin(9600);


  pinMode(LED_BUILTIN, OUTPUT); //added by drc
  Wire.setSDA(18); //use a 4.7k pullup resistor //added by drc
  Wire.setSCL(19); //use a 4.7k pullup resistor //added by drc
  

  //while (!Serial) { // adding a delay to avoid issues seen in other forum posts.
  //  delay(10);
 // }
  
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D (verify this is true for the generic MPR121, and cut ground trade on addr pin backside)
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  if (!cap2.begin(0x5B)) {
Serial.println("MPR121 B not found, check wiring?");
while (1);
}
Serial.println("MPR121 B found!");
}

void loop() {
  // Get the currently touched pads
  currtouched = cap.touched();
  currtouched2 = cap2.touched();
 

  /*This is where we'll change from sending notes to CC values.  
  It currently sends a note off/control value of 0 when the key is released.  
  I think we may delete the noteOff portion, and just send the CC message, 
  and specify in an array whether it is 0 or 127 for the controlValue.. 
  */

  // For mpr121 0X5A---------------------------------
  for (uint8_t i=0; i<12; i++) { //changed i<0 to i<numElectrodes
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {

      digitalWrite (LED_BUILTIN, HIGH); //added by drc
      //usbMIDI.sendNoteOn(notes[i], 127, channel); // added by drc, declared channel with #, still returning ch 15 in midi monitor?
      usbMIDI.sendControlChange(controlNumA[i], 127, channel); //(control#, controlval, channel)

      Serial.print(i); Serial.println(" touched of A");
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {

      digitalWrite (LED_BUILTIN, LOW);
      //usbMIDI.sendNoteOff(notes[i], 127, channel); // maximum velocity
      usbMIDI.sendControlChange(controlNumA[i], 0, channel); //(control#, controlval, channel)


      Serial.print(i); Serial.println(" released of A");
    }

// For mpr121 0x5B---------------------------------
    if ((currtouched2 & _BV(i)) && !(lasttouched2 & _BV(i)) ) {

      digitalWrite (LED_BUILTIN, HIGH); //added by drc
      //usbMIDI.sendNoteOn(notes[i], 127, channel); // added by drc, declared channel with #, still returning ch 15 in midi monitor?
      usbMIDI.sendControlChange(controlNumB[i], 127, channel); //(control#, controlval, channel)

      Serial.print(i); Serial.println(" touched of B");
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched2 & _BV(i)) && (lasttouched2 & _BV(i)) ) {

      digitalWrite (LED_BUILTIN, LOW);
      //usbMIDI.sendNoteOff(notes[i], 127, channel); // maximum velocity
      usbMIDI.sendControlChange(controlNumB[i], 0, channel); //(control#, controlval, channel)


      Serial.print(i); Serial.println(" released of B");
    }
  }

  // reset our state
  lasttouched = currtouched;
  lasttouched2 = currtouched2;

  // comment out this line for detailed data from the sensor!
  return;
  
  
  // debugging info, what
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t\t 0x"); Serial.println(cap.touched(), HEX);
  Serial.print("Filt: ");
  for (uint8_t i=0; i<12; i++) {
    Serial.print(cap.filteredData(i)); Serial.print("\t");
  }
  Serial.println();
  Serial.print("Base: ");
  for (uint8_t i=0; i<12; i++) {
    Serial.print(cap.baselineData(i)); Serial.print("\t");
  }
  Serial.println();
  
  
  // put a delay so it isn't overwhelming
  delay(100);
}
