// PURPOSE: this code file will implement the morse code trainer game with Arduino
#include <stdint.h>
#include "morse_lookup.h"
#include "word_bank.h"

// TIMER PINS (BLUE LEDS)
const int LED1 = 4; // turns off at 8s
const int LED2 = 5; // turns off at 16s
const int LED3 = 6; // turns off at 24s

// INTERRUPT PINS
/*
    - four buttons will go into pin 2 of the arduino through OR-gate output
    - read inside ISR to identify which button is fired
*/

const int intPin = 2; // OR-gate output of all 4 game buttons
const int nextLevelButton = 7; // button to advance to next difficulty level
const int backLevelButton = 8; // button to go back to the previous difficulty level
const int resetButton = 9; // button to reset game
const int skipWordButton = 10; // button to skip current word on current level

const int morseInput = 3; // morse code input button (not interrupt)

// declaring global variables
volatile uint32_t tickCount = 0;
volatile uint32_t lastEdgeTime = 0;
volatile uint8_t gameButtonId = 0; // which game button fired (1-4), 0 = none
volatile bool gameButtonEvent = false;

uint32_t pressStart;
uint32_t lastRelease;
uint32_t wordStartTime = 0;
uint32_t msgStartTime = 0;

const int dotDashThreshold = 200; // 200 ms
const int gapThreshold = 800; // 800 ms silence -> commit letter
const uint32_t wordTimeout = 24000; // 24 second word deadline
const uint32_t displayHold = 2000; // 2 second message display
const uint32_t debounceMs = 30;
const uint32_t minPressMs = 20;

int buttonState;
int lastButtonState = LOW;
char letterBuf[8];
int letterLen = 0;
char symbol;

// game level constraints
const int beginner = 0;
const int medium = 1;
const int hard = 2;
int currentLevel = beginner;
const char* targetWord = nullptr; // word player is currently trying to enter
char typedWord[6];
int typedLen = 0; // word length counter

// position counters
int beginnerPos = 0;
int mediumPos = 0;
int hardPos = 0;

// order arrays sized by their word banks
int beginnerOrder[beginnerCount];
int mediumOrder[mediumCount];
int hardOrder[hardCount];

int correctCount = 0;
int lives = 3;
const int wordsToAdvance = 3;

// state machine
const int STATE_PLAYING  = 0;
const int STATE_SHOW_MSG = 1;
const int STATE_GAME_OVER = 2;
int gameState = STATE_PLAYING;
char msgBuf[64];

// shuffle helper function
void shuffleOrder(int order[], int count){
    for(int i = count - 1; i > 0; i--){
        int j = random(0, i+1);
        int temp = order[i];
        order[i] = order[j];
        order[j] = temp;
    }
}

void setup() {
    Serial.begin(115200); // initialize serial
    
    // initial buffers
    letterLen = 0;
    letterBuf[0] = '\0';
    lastRelease = 0;

    // game buffers
    typedLen = 0;
    typedWord[0] = '\0';

    // fill order arrays
    for(int i = 0; i < beginnerCount; i++){
        beginnerOrder[i] = i;
    }
    for(int i = 0; i < mediumCount; i++){
        mediumOrder[i] = i;
    }
    for(int i = 0; i < hardCount; i++){
        hardOrder[i] = i;
    }

    // shuffle order arrays
    shuffleOrder(beginnerOrder, beginnerCount);
    shuffleOrder(mediumOrder, mediumCount);
    shuffleOrder(hardOrder, hardCount);

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

// ISR function for timer
ISR(TIMER2_COMPA_vect) {
    tickCount++;
}

void loop() {
    // read current button state
    buttonState = digitalRead(buttonPin);

    // start beginner level
    if (currentLevel == beginner && targetWord == nullptr) {
        targetWord = beginnerWords[beginnerOrder[beginnerPos]];
        beginnerPos++;
        Serial.print("Target word: ");
        Serial.println(targetWord);
    }

    // gap condition (nothing being pressed to insert new letter)
    if(buttonState == LOW && letterLen > 0 && (tickCount - lastRelease) >= gapThreshold){
        char decoded = decodeMorse(letterBuf);
        
        if (typedLen < (int)sizeof(typedWord) - 1) {
            typedWord[typedLen++] = decoded;
            typedWord[typedLen] = '\0';
        }

        Serial.print("Decoded: ");
        Serial.println(decoded);

        // check if word is complete and correct
        if (strcmp(typedWord, targetWord) == 0) {
            correctCount++;
            Serial.print("Correct! Number of Correct Words: ");
            Serial.println(correctCount);

            if (correctCount >= 3) {
                // advance to next level
                if (currentLevel == beginner) {
                    currentLevel = medium;
                    correctCount = 0;
                    Serial.println("Congrats! Player advanced to MEDIUM!");
                } else if (currentLevel == medium) {
                    currentLevel = hard;
                    correctCount = 0;
                    Serial.println("Congrats! Player advanced to HARD!");
                }
            }

            // get next word
            if (currentLevel == beginner) {
                targetWord = beginnerWords[beginnerOrder[beginnerPos % beginnerCount]];
                beginnerPos++;
            } else if (currentLevel == medium) {
                targetWord = mediumWords[mediumOrder[mediumPos % mediumCount]];
                mediumPos++;
            } else if (currentLevel == hard) {
                targetWord = hardWords[hardOrder[hardPos % hardCount]];
                hardPos++;
            }

            typedLen = 0;
            typedWord[0] = '\0';
        }

        // clear buffer for next letter
        letterLen = 0;
        letterBuf[0] = '\0';
    }

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