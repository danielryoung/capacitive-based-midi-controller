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
//current issues: doesn't work once unplugged and plugged back in.  only works when connedted to the serial monitor

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include <Ticker.h>
#include <FastLED.h>

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

//This is the duration between sends of midi signal in Milliseconds.
#define TRIGGER_DURATION 100

// LED Stuff

#define DATA_PIN    3
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    64
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120
bool ambient_leds = true;
bool trigger_leds = false;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns from Demo Reel 100 example

 

void triggerLoop(); // hoisted, defined below.
void ledFrameLoop();
void startAmbient();

#define LED_DECAY 5000 // leds will decay in brightness for 5 seconds before going dark
#define AMBIENT_DELAY 10000 // leds will start doing something after the keys are untouched for this duration.

// Ticker Library sets up timers and a function to call when the timer elapses
           //(functioncalled, timertime, number to repeat(0is forever, RESOLUTION)
Ticker repeatTimer(triggerLoop, TRIGGER_DURATION, 0, MILLIS);

Ticker ledFrameTimer(ledFrameLoop, 1000/FRAMES_PER_SECOND, 0, MILLIS);
Ticker ambientLEDs(startAmbient, AMBIENT_DELAY, 0 , Ticker.MILLIS);
// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 capA = Adafruit_MPR121();
Adafruit_MPR121 capB = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouchedA = 0;
uint16_t currtouchedA = 0;
uint16_t lasttouchedB = 0;
uint16_t currtouchedB = 0;
const uint8_t numElectrodes = 12; //added by drc
const uint8_t notes[numElectrodes] = {36, 38, 40, 43, 45, 47, 48, 50, 52, 55, 57, 60}; //added by drc
const uint8_t controlNumA[] = {88,88,87,87,13,13,5,5,0,0,3,3}; //Change to #'s stefan is using. 88, 87, 13, 5, 0, 3, 50 is mode shift
const uint8_t controlNumB[] = {88,88,87,87,13,13,5,5,0,0,3,3}; //Change to #'s stefan is using. 88, 87, 13, 5, 0, 3, 50 is mode shift

const uint8_t controlValA[] = {127,0,127,0,127,0,127,0,127,0,127,0}; //Change to #'s stefan is using
const uint8_t controlValB[] = {127,0,127,0,127,0,127,0,127,0,127,0}; //Change to #'s stefan is using

uint8_t ElectrodeTouchedA[numElectrodes] = {0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t ElectrodeTouchedB[numElectrodes] = {0,0,0,0,0,0,0,0,0,0,0,0};

int channel = 1; //added by drc // channel 0 was returning channel 16 in midi monitor

 


void setup() {
  Wire.begin(0x5A); //added by drc
  Wire.begin(0x5B); //added by drc

  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT); //added by drc
  Wire.setSDA(18); //use a 4.7k pullup resistor //added by drc
  Wire.setSCL(19); //use a 4.7k pullup resistor //added by drc
  
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!capA.begin(0x5A)) {
    Serial.println("MPR121 0x5A not found, check wiring?");
    while (1);
  }
    Serial.println("MPR121 0x5A found!");

    if (!capA.begin(0x5B)) {
    Serial.println("MPR121 0x5B not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 0x5B found!");
  
  repeatTimer.start();
  ledFrameTimer.start();

    // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  
}

void loop() {
  // Get the currently touched pads
  currtouchedA = capA.touched();
  currtouchedB = capB.touched();

  // key repeat timer
  repeatTimer.update();

  // led frame render timer
  ledFrameTimer.update();

  // turn on ambient LEDs timer
  ambientLEDs.update();
  FastLED.show(); 
  
  // timer is runnig as long as this is getting called.

  checkElectrodes();
}

void checkElectrodes(){

//For mpr121 0x5A--------------------
  for (uint8_t i=0; i<numElectrodes; i++) { //changed i<0 to i<numElectrodes
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouchedA & _BV(i)) && !(lasttouchedA & _BV(i)) ) {


      digitalWrite (LED_BUILTIN, HIGH); //added by drc

      Serial.print(i); Serial.println(" touched of A");
      // set the array value to 1 on touch
      ElectrodeTouchedA[i] = 1;
    }
    // if it *was* touched and now *isnt*, alert! 
    if (!(currtouchedA & _BV(i)) && (lasttouchedA & _BV(i)) ) {

      digitalWrite (LED_BUILTIN, LOW);
      Serial.print(i); Serial.println(" released of A");
      
      // set it back to 0 on release
      ElectrodeTouchedA[i] = 0;   
    }
  }

//For mpr121 0x5B--------------------
  for (uint8_t i=0; i<numElectrodes; i++) { //changed i<0 to i<numElectrodes
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouchedB & _BV(i)) && !(lasttouchedB & _BV(i)) ) {


      digitalWrite (LED_BUILTIN, HIGH); //added by drc

      Serial.print(i); Serial.println(" touched of B");
      // set the array value to 1 on touch
      ElectrodeTouchedB[i] = 1;

      // on touch, turn off ambient leds and turn on trigger_leds
      ambient_leds = false;
      trigger_leds = true;
      
    }
    // if it *was* touched and now *isnt*, alert! 
    if (!(currtouchedB & _BV(i)) && (lasttouchedB & _BV(i)) ) {

      digitalWrite (LED_BUILTIN, LOW);
      Serial.print(i); Serial.println(" released of B");
      // set it back to 0 on release
      ElectrodeTouchedB[i] = 0;   
      // start the ambient timer after release
      // this will start counting from release and fire after AMBIENT_DELAY
      ambientLEDs.start();
    }
  }

  // reset our state
  lasttouchedA = currtouchedA;
  lasttouchedB = currtouchedB;

}

void triggerLoop(){
  // this fires on a timer and anything pressed triggers midi
  for (uint8_t i=0; i<numElectrodes; i++) { 
    if (ElectrodeTouchedA[i]) {

      triggerMidiA(i);
    }
     if (ElectrodeTouchedB[i]) {
      triggerMidiB(i);
    }
  }
  //Serial.print(ElectrodeTouched[0]);
  //Serial.print("trigger Loop");

}

void startAmbient(){
  // this fires after the ambient timer has elapsed.
  // it will start the leds doing the ambient pattern until set to false.
  
  ambient_leds = true;
}

void ledFrameLoop(){

  // calls bpm from DEMO REEL 100
  // This is a single 'frame' of leds for the whole strip.
  // it gets called based on the timer to set framerate
  // any function that writes a full strip worth of colors will work.

  gHue++;
  if (trigger_leds == true) {
    bpm();
  }
  else if (ambient_leds == true && trigger_leds == false) {
    rainbow();
  }
  
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}



void triggerMidiA(int i){
   usbMIDI.sendControlChange(controlNumA[i], controlValA[i], channel); //(control#, controlval, channel)
   Serial.print("triggered midi on: A ");
   Serial.println(i);
}
void triggerMidiB(int i){
   usbMIDI.sendControlChange(controlNumB[i], controlValB[i], channel); //(control#, controlval, channel)
   Serial.print("triggered midi on: B ");
   Serial.println(i);
}
/* 
void noteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
	usbMIDI.sendNoteOn(notes[numElectrodes], velocity, channel); //notes changed to [numElectrodes] by drc
}

void noteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {
	usbMIDI.sendNoteOff(notes[numElectrodes], velocity, channel);
}
*/
