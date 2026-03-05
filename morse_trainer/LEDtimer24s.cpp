const int LED1 = 10;   // turns off at 8s
const int LED2 = 11;   // turns off at 16s
const int LED3 = 12;   // turns off at 24s
const int correct = 2; // interrupt pin 

const unsigned long time_LED     = 8000;  // 8 seconds per LED
const unsigned long time_out = 2000;  // keep all LEDs OFF for 2s if user fails

// Interrupt
// ISR sets to true when correct word is inputted
volatile bool correctWord = false;

// Optional simple debounce (in ISR). 
// volatile unsigned long lastIsrMs = 0;
// const unsigned long DEBOUNCE_MS = 50;

// State 
enum Phase { RUNNING, FAIL };
Phase phase = RUNNING;

unsigned long phaseStartMs = 0;   // start time of current phase (RUNNING or FAIL)

void setAllLeds(bool on) {
  if (on == true) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
  }
  else {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
  }
}

void resetGameTimer() {
  phase = RUNNING;
  phaseStartMs = millis();
  setAllLeds(true);
  correctWord = false; // clear after handling
}

// ISR: fires when correct word is entered!!!
void submitISR() {
  correctWord = true;
}

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  // Button wiring for SUBMIT_PIN:
  // - One side of button to SUBMIT_PIN (2)
  // - Other side of button to GND
  // - Use internal pullup:
  pinMode(correct, INPUT_PULLUP);

  // Interrupt triggers on button press (HIGH->LOW) because of pullup
  attachInterrupt(digitalPinToInterrupt(correct), submitISR, FALLING);

  resetGameTimer();
}

void loop() {
  unsigned long now = millis();

  // If correct word event happened at ANY time: reset immediately
  if (correctWord) {
    resetGameTimer();
    return;
  }

  if (phase == RUNNING) {
    unsigned long elapsed = now - phaseStartMs;

    // LED countdown logic (non-blocking)
    if (elapsed < time_LED) {
      // 0-8s: all on
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, HIGH);
    } 
    else if (elapsed < 2 * time_LED) {
      // 8-16s: LED1 off
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, HIGH);
    } 
    else if (elapsed < 3 * time_LED) {
      // 16-24s: LED1, LED2 off
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, HIGH);
    } 
    else {
      // 24s reached with no correct input:
      // turn all off and hold off for 2s
      setAllLeds(false);
      phase = FAIL;
      phaseStartMs = now;
    }

  } else if (phase == FAIL) {
    // All LEDs OFF for 2s, instant reset
    if (now - phaseStartMs >= time_out) {
      resetGameTimer();
      return;
    }
  }
}
