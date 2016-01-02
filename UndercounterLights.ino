/*
 * UndercounterLights.ino
 * 2015 WLWilliams
 * 
 * This sketch controls a DotStar LED strip for under counter lights.
 * A light sensor is used to determine if it is dark enough
 * A motion sensor turns on the LEDs
 */
 
#include "FastLED.h"
#include <SPI.h>
#include "RF24.h"

// How many leds in your strip?
#define NUM_LEDS            39 
#define UPDATES_PER_SECOND 100
#define DATA_PIN             4  // White
#define CLOCK_PIN            5  // Blue
#define SELF_TEST_BUTTON     6
#define PIR_INPUT           12
#define BLINK_LED           13
#define LIGHT_SENSOR_INPUT  A0
#define MIN_VALUE_FOR_ON    250   // Values from 0 to 1023 (full brightness)

CRGB          leds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType    currentBlending;

void setup() { 
  Serial.begin(9600);
  
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
	FastLED.setBrightness(24);

  pinMode(BLINK_LED,OUTPUT);
  pinMode(PIR_INPUT,INPUT);
  pinMode(LIGHT_SENSOR_INPUT,INPUT);
  pinMode(SELF_TEST_BUTTON,INPUT_PULLUP);
}

void loop() 
{ 
  if ( StateMachine() )
    digitalWrite(BLINK_LED,millis() % 2000 > 1000);      // Not in Selftest
  else
    digitalWrite(BLINK_LED,millis() % 500 > 250);        // In Selftest - blink faster

    FastLED.delay(1000 / UPDATES_PER_SECOND);
}

typedef enum LedState { LEDS_OFF,  START, IN_SELFTEST, MOTION, DELAY_TO_OFF } LedState;

bool StateMachine ( void )
{
  static LedState state = LEDS_OFF;
  static unsigned long timer;
  switch ( state )
  {
    case LEDS_OFF:
      FastLED.clear();
      FastLED.show();
      state = START;  
      //DoLEDs(true);  
      break; 
    case START:
      if ( StartSelfTest() )  {
        timer = millis();
        state = IN_SELFTEST;
      }
      else if ( ! LightsOn() && MotionDetected() )
        state = MOTION;  
      break;
    case IN_SELFTEST:
      DoLEDs(false);
      if ( millis() - timer > 10000UL ) 
        state = LEDS_OFF;
      break;
    case MOTION:
      DoLEDs(false);
      if ( LightsOn() || StartSelfTest() )
        state = LEDS_OFF;
      else if ( ! MotionDetected() ) {
        timer = millis();
        state = DELAY_TO_OFF;
      }
      break;
    case DELAY_TO_OFF:
      DoLEDs(false);
      if ( StartSelfTest() || LightsOn() || (millis() - timer > 30000) )
        state = LEDS_OFF;
      else if ( MotionDetected() )  
        state = MOTION;
      break;
  }
  return state != IN_SELFTEST;
}

void DoLEDs ( bool reset ) {
  unsigned long secondHand = millis() % 30000UL;
    if ( secondHand <  15000UL )        colorChase(false);
    else if ( secondHand <  29990UL )   sineWaveColor(false);
  FastLED.show();
}

inline bool StartSelfTest ( void ) {
  return digitalRead(SELF_TEST_BUTTON) == LOW;
}

inline bool LightsOn ( void ) {
  return analogRead(LIGHT_SENSOR_INPUT) > MIN_VALUE_FOR_ON;    // Min value for light. range 0 to 1023
}

inline bool MotionDetected ( void ) {
  return digitalRead(PIR_INPUT) == HIGH;
}

void colorChase ( bool restart )
{
  static uint8_t Index = -1;
  if ( restart ) Index = -1;
  Index = Index + 1;

  uint8_t colorIndex;
  uint8_t brightness = 255;

  colorIndex = Index;
  for ( int i = 0; i < NUM_LEDS; i++ ) 
  {
    leds[i] = ColorFromPalette( CloudColors_p, colorIndex, 48, LINEARBLEND);
    colorIndex += 5;
  }
}

void sineWaveColor ( bool restart )
{
  #define Period 6.0      // in seconds
  const double w = 2.0 * 3.1415926 / Period;  //
  
  uint8_t val = (uint8_t)(64.0 * 
      ( 1.0 + sin( w * (double)(millis()) / 1000.0) ) );
  
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, val);
  }
}

