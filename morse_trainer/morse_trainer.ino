#include <stdint.h>
#include "morse_lookup.h"
// PURPOSE: this code file will implement the morse code trainer game with Arduino

// declaring global variables
volatile uint32_t tickCount = 0; // unsigned long integer
uint32_t pressStart;
uint32_t duration;
uint32_t lastRelease;
uint32_t lastEdgeTime = 0; 
const int buttonPin = 4;
const int dotDashThreshold = 200; // 200 ms
const int gapThreshold = 800; // 800 ms
const uint32_t debounceMs = 30;
const uint32_t minPressMs = 20;
int buttonState;
int lastButtonState = LOW;
char letterBuf[8];
int letterLen = 0;
char symbol;

void setup() {
    Serial.begin(115200); // initialize serial
    
    // initial buffers
    letterLen = 0;
    letterBuf[0] = '\0';
    lastRelease = 0;

    noInterrupts(); // disable interrupts during setup
    
    // reset timer2 registers to 0
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2  = 0;

    // configure timer2 mode to CTC
    TCCR2A |= (1 << WGM21); // OR a bit into TCCR2A 

    // set compare match register (OCR2A) for 1 ms intervals
    // (16 MHz / (64 * 0.001s)) - 1 = 249
    OCR2A = 249;
    TCCR2B |= (1 << CS22);

    // enable timer2 compare match
    TIMSK2 |= (1 << OCIE2A);

    interrupts(); // enable interrupts

    // set up button pin
    pinMode(buttonPin, INPUT);
}

// ISR function
ISR(TIMER2_COMPA_vect) {
    tickCount++;
}

void loop() {
    /*
    // testing interrupts and counter
    static uint32_t last = 0;
    if(tickCount - last >= 1000){
        Serial.println(tickCount);
        last += 1000;
    }
    */

    // read current button state
    buttonState = digitalRead(buttonPin);

    // if button is turned on
    if (lastButtonState == LOW && buttonState == HIGH) {
        uint32_t now = tickCount;
        if (now - lastEdgeTime >= debounceMs) {
        lastEdgeTime = now;
        pressStart = now;
        }
    }

    // if button is turned off
    if(lastButtonState==HIGH && buttonState==LOW){
        uint32_t now = tickCount;
    if (now - lastEdgeTime >= debounceMs) {
      lastEdgeTime = now;

      uint32_t duration = now - pressStart;

      // ignore very short taps/outside world noise
      if (duration >= minPressMs) {
        char symbol = (duration < dotDashThreshold) ? '.' : '-';

        // append to buffer
        if (letterLen < (int)sizeof(letterBuf) - 1) {
          letterBuf[letterLen++] = symbol;
          letterBuf[letterLen] = '\0';
        }

        lastRelease = now;

        // test prints
        Serial.print("Symbol: ");
        Serial.println(symbol);
        Serial.print("Current buffer: ");
        Serial.println(letterBuf);
      }
    }
  }
  lastButtonState = buttonState; // update the last button state

  // gap condition (nothing being pressed to insert new letter)
  if(buttonState == LOW && letterLen > 0 && (tickCount - lastRelease) >= gapThreshold){
    char decoded = decodeMorse(letterBuf);

    Serial.print("Decoded: ");
    Serial.println(decoded);

    // clear buffer for next letter
    letterLen = 0;
    letterBuf[0] = '\0';
   }
}