// PURPOSE: this code file will implement the morse code trainer game with Arduino
#include <stdint.h>
#include "morse_lookup.h"
#include "word_bank.h"

// TIMER PINS (BLUE LEDS)
const int LED1 = 4; // turns off at 8s
const int LED2 = 5; // turns off at 16s
const int LED3 = 6; // turns off at 24s

// HEALTH PINS (RED LEDS)
const int LIFE1 = 11;
const int LIFE2 = 12;
const int LIFE3 = A0;

// INTERRUPT PINS
//    - four buttons will go into pin 2 of the arduino through OR-gate output
//    - read inside ISR to identify which button is fired

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
uint32_t failStartTime = 0;

const int dotDashThreshold = 200; // 200 ms
const int gapThreshold = 800; // 800 ms silence -> commit letter
const uint32_t wordTimeout = 24000; // 24 second word deadline
const uint32_t displayHold = 2000; // 2 second message display
const uint32_t failHold = 1000;
const uint32_t debounceMs = 30;
const uint32_t minPressMs = 20;
const uint32_t flashInterval = 300; // ms between flashes on game over

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
const int STATE_FAIL = 2;
const int STATE_GAME_OVER = 3;
int gameState = STATE_PLAYING;
char msgBuf[64];

// game over flash state
bool gameOverActive = false;
bool flashState = false;
uint32_t lastFlashTime = 0;

// game start
bool gameStarted = false;

// shuffle helper function
void shuffleOrder(int order[], int count){
    for(int i = count - 1; i > 0; i--){
        int j = random(0, i+1);
        int temp = order[i];
        order[i] = order[j];
        order[j] = temp;
    }
}

// -------------------------------------
// --------- HELPER FUNCTIONS ----------
// -------------------------------------

// ---- LED helpers ----
void setTimerLEDs(bool on) {
    digitalWrite(LED1, on ? HIGH : LOW);
    digitalWrite(LED2, on ? HIGH : LOW);
    digitalWrite(LED3, on ? HIGH : LOW);
}

void setLivesLEDs(bool on) {
    digitalWrite(LIFE1, on ? HIGH : LOW);
    digitalWrite(LIFE2, on ? HIGH : LOW);
    digitalWrite(LIFE3, on ? HIGH : LOW);
}

void updateLives() {
    if (lives >= 3) {
        digitalWrite(LIFE1, HIGH);
        digitalWrite(LIFE2, HIGH);
        digitalWrite(LIFE3, HIGH);
    } else if (lives == 2) {
        digitalWrite(LIFE1, LOW);
        digitalWrite(LIFE2, HIGH);
        digitalWrite(LIFE3, HIGH);
    } else if (lives == 1) {
        digitalWrite(LIFE1, LOW);
        digitalWrite(LIFE2, LOW);
        digitalWrite(LIFE3, HIGH);
    } else {
        digitalWrite(LIFE1, LOW);
        digitalWrite(LIFE2, LOW);
        digitalWrite(LIFE3, LOW);
    }
}

void handleGameOverFlash() {
    if ((tickCount - lastFlashTime) >= flashInterval) {
        lastFlashTime = tickCount;
        flashState = !flashState;
        setTimerLEDs(flashState);
        setLivesLEDs(flashState);
    }
}

void showMessage(const char* msg) {
    strncpy(msgBuf, msg, sizeof(msgBuf) - 1);
    msgBuf[sizeof(msgBuf) - 1] = '\0';
    Serial.println(msgBuf);
    msgStartTime = tickCount;
    gameState = STATE_SHOW_MSG;
}

void updateLEDs() {
    uint32_t elapsed = tickCount - wordStartTime;
    // LED1 off at 8s, LED2 off at 16s, LED3 off at 24s
    if (elapsed < 8000) {
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, HIGH);
        digitalWrite(LED3, HIGH);
    } else if (elapsed < 16000) {
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, HIGH);
        digitalWrite(LED3, HIGH);
    } else if (elapsed < 24000) {
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, HIGH);
    }
}

void nextWord() {
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

    wordStartTime = tickCount;
    typedLen = 0;
    typedWord[0] = '\0';
    letterLen = 0;
    letterBuf[0] = '\0';

    // reset timer LEDs for new word
    setTimerLEDs(true);

    Serial.print("Target word: ");
    Serial.println(targetWord);
    Serial.print("Lives: ");
    Serial.println(lives);
}

void resetGame() {
    lives = 3;
    correctCount = 0;
    currentLevel = beginner;
    beginnerPos = 0;
    mediumPos = 0;
    hardPos = 0;
    gameOverActive = false;
    flashState = false;
    gameStarted = false; // require morse button press to restart
    gameState = STATE_PLAYING;
    targetWord = nullptr;

    shuffleOrder(beginnerOrder, beginnerCount);
    shuffleOrder(mediumOrder, mediumCount);
    shuffleOrder(hardOrder, hardCount);

    updateLives();
    setTimerLEDs(false);

    Serial.println("Game reset! Press morse button to start.");
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
    for (int i = 0; i < beginnerCount; i++) {
        beginnerOrder[i] = i;
    }
    for (int i = 0; i < mediumCount; i++) {
        mediumOrder[i] = i;
    }
    for (int i = 0; i < hardCount; i++) {
        hardOrder[i] = i;
    }

    // shuffle order arrays
    randomSeed(analogRead(A5));
    shuffleOrder(beginnerOrder, beginnerCount);
    shuffleOrder(mediumOrder, mediumCount);
    shuffleOrder(hardOrder, hardCount);

    // set up pins
    pinMode(morseInput, INPUT);
    pinMode(intPin, INPUT);
    pinMode(nextLevelButton, INPUT);
    pinMode(backLevelButton, INPUT);
    pinMode(resetButton, INPUT);
    pinMode(skipWordButton, INPUT);

    // led pins
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LIFE1, OUTPUT);
    pinMode(LIFE2, OUTPUT);
    pinMode(LIFE3, OUTPUT);

    // timer LEDs off until game starts
    setTimerLEDs(false);
    updateLives();

    Serial.println("Press morse button to start!");

    cli(); // disables the arduino timers
    
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

    sei(); // enable timer

    // attach interrupt to pin 2, fires on the button
    attachInterrupt(digitalPinToInterrupt(intPin), gameButtonISR, CHANGE);
}

// ISR function for timer2 (fire every 1 ms)
ISR(TIMER2_COMPA_vect) {
    tickCount++; // number of milliseconds since program started
}

// ISR for OR-gated game buttons on pin 2
// read individual sense pins to identify which button triggered the interrupt
void gameButtonISR() {
    uint32_t now = tickCount;

    // debounce: ignore edges too close together
    if ((now - lastEdgeTime) < debounceMs) {
        return;
    }
    lastEdgeTime = now;

    // only act on rising edge (button pressed down)
    if (digitalRead(intPin) == LOW) {
        return;
    }

    // identify which button by reading individual sense pins
    if (digitalRead(nextLevelButton)) {
        gameButtonId = 1;
    } else if (digitalRead(backLevelButton)) {
        gameButtonId = 2;
    } else if (digitalRead(resetButton)) {
        gameButtonId = 3;
    } else if (digitalRead(skipWordButton)) {
        gameButtonId = 4;
    }

    gameButtonEvent = true;
}

void loop() {

    // GAME START: press morse button to begin
    if (!gameStarted) {
        if (digitalRead(morseInput) == HIGH) {
            gameStarted = true;
            Serial.println("Game started!");
            nextWord();
            // wait for release so the press doesn't count as a morse symbol
            while (digitalRead(morseInput) == HIGH) {}
        }
        return;
    }
    
    // GAME OVER: flash timer and lives LEDs until reset button is pressed
    if (gameOverActive) {
        handleGameOverFlash();

        if (gameButtonEvent && gameButtonId == 3) {
            gameButtonEvent = false;
            resetGame();
        }
        return;
    }

    // FAIL STATE: all LEDs off briefly after timeout, then next word presented
    if (gameState == STATE_FAIL) {
        if ((tickCount - failStartTime) >= failHold) {
            gameState = STATE_PLAYING;
            nextWord();
        }
        return;
    }

    // SHOW MESSAGE: hold for displayHold ms then resume
    if (gameState == STATE_SHOW_MSG) {
        if ((tickCount - msgStartTime) >= displayHold) {
            if (lives <= 0) {
                gameOverActive = true;
                flashState = true;
                lastFlashTime = tickCount;
                setTimerLEDs(false);
                Serial.println("GAME OVER!");
                return;
            }
            gameState = STATE_PLAYING;
            nextWord();
        }
        return;
    }

    // PLAYING GAME:

    // start beginner level on first run
    if (currentLevel == beginner && targetWord == nullptr) {
        nextWord();
    }

    // handle game button events from ISR
    if (gameButtonEvent) {
        gameButtonEvent = false;

        if (gameButtonId == 1) {
            // nextLevelButton - advance level
            if (currentLevel < hard) {
                currentLevel++;
                correctCount = 0;
                if (currentLevel == medium) Serial.println("Level advanced to: MEDIUM!");
                else if (currentLevel == hard) Serial.println("Level advanced to: HARD!");
                nextWord();
            } else {
                Serial.println("Already at hardest level!");
            }

        } else if (gameButtonId == 2) {
        // backLevelButton - go back a level
            if (currentLevel > beginner) {
                currentLevel--;
                correctCount = 0;
                if (currentLevel == beginner) Serial.println("Level back to: BEGINNER!");
                else if (currentLevel == medium) Serial.println("Level back to: MEDIUM!");
                nextWord();
            } else {
                Serial.println("Already at easiest level!");
            }

        } else if (gameButtonId == 3) {
            // resetButton - reset entire game
            resetGame();

        } else if (gameButtonId == 4) {
            // skipWordButton - skip current word, lose a life
            lives--;
            updateLives();
            Serial.print("Word skipped! Lives remaining: ");
            Serial.println(lives);
            showMessage(lives <= 0 ? "GAME OVER!" : "Word skipped!");
        }
    }

    // update timer LEDs based on elasped time
    updateLEDs();

    // read current button state
    buttonState = digitalRead(morseInput);

    // morse button being pressed
    if (lastButtonState == LOW && buttonState == HIGH) {
        pressStart = tickCount;
    }

    // morse button released
    if (lastButtonState == HIGH && buttonState == LOW) {
        uint32_t duration = tickCount - pressStart;

        // ignore very short taps
        if (duration >= minPressMs) {
            symbol = (duration < dotDashThreshold) ? '.' : '-';

            // append to buffer
            if (letterLen < (int)sizeof(letterBuf) - 1) {
                letterBuf[letterLen++] = symbol;
                letterBuf[letterLen] = '\0';
            }

            lastRelease = tickCount;

            // test prints
            Serial.print("Symbol: ");
            Serial.println(symbol);
            Serial.print("Current buffer: ");
            Serial.println(letterBuf);
        }
    }

    lastButtonState = buttonState; // update last button state

    // gap condition (nothing being pressed -> commit letter)
    if (buttonState == LOW && letterLen > 0 && (tickCount - lastRelease) >= gapThreshold) {
        char decoded = decodeMorse(letterBuf);

        if (typedLen < (int)sizeof(typedWord) - 1) {
            typedWord[typedLen++] = decoded;
            typedWord[typedLen] = '\0';
        }

        Serial.print("Decoded: ");
        Serial.println(decoded);

        Serial.print("Word so far: ");
        Serial.println(typedWord);

        // clear buffer for next letter
        letterLen = 0;
        letterBuf[0] = '\0';

        // check if word is complete
        if (typedLen == (int)strlen(targetWord)) {
            if (strcmp(typedWord, targetWord) == 0) {
                // correct word
                correctCount++;
                Serial.print("Correct! Number of Correct Words: ");
                Serial.println(correctCount);

                if (correctCount >= wordsToAdvance) {
                    correctCount = 0;
                    if (currentLevel == beginner) {
                        currentLevel = medium;
                        showMessage("Congrats! Player advanced to MEDIUM!");
                    } else if (currentLevel == medium) {
                        currentLevel = hard;
                        showMessage("Congrats! Player advanced to HARD!");
                    } else {
                        // won the game
                        gameOverActive = true;
                        flashState = true;
                        lastFlashTime = tickCount;
                        Serial.println("YOU WIN! All levels cleared!");
                    }
                } else {
                    showMessage("Correct!");
                }

            } else {
                // wrong word
                lives--;
                updateLives();
                Serial.print("Wrong! Typed: ");
                Serial.print(typedWord);
                Serial.print(" Target: ");
                Serial.println(targetWord);
                Serial.print("Lives remaining: ");
                Serial.println(lives);

                showMessage(lives <= 0 ? "GAME OVER!" : "Wrong word! Try again.");
            }

            typedLen = 0;
            typedWord[0] = '\0';
        }
    }

    // word timeout - 24 seconds
    if ((tickCount - wordStartTime) >= wordTimeout) {
        lives--;
        updateLives();
        Serial.print("Time's up! Lives remaining: ");
        Serial.println(lives);

        if (lives <= 0) {
            gameOverActive = true;
            flashState = true;
            lastFlashTime = tickCount;
            setTimerLEDs(false);
            Serial.println("GAME OVER!");
        } else {
            // enter fail state - all LEDs off briefly before next word
            setTimerLEDs(false);
            failStartTime = tickCount;
            gameState = STATE_FAIL;
        }
    }
}