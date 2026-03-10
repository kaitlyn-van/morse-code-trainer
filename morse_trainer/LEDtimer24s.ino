//TIMER DEFINITIONS
const int LED1 = 12;        // turns off at 8s
const int LED2 = 11;        // turns off at 16s
const int LED3 = 10;        // turns off at 24s
const int morse = 13;       // morse code input button
const int reset_game = 2;   // interrupt pin
const int skip = 3;         // interrupt pin

//LIVES DEFINITIONS
const int LIFE1 = 4;        
const int LIFE2 = 5;        
const int LIFE3 = 6;
int lifeCount = 3;

//POINTS DEFINITION
const int POINT1 = 7; 
const int POINT2 = 8; 
const int POINT3 = 9; 
int pointcount = 0;

//game over definitions
bool gameOverActive = false;
unsigned long lastFlashMs = 0;
bool flashState = false;
const unsigned long flashInterval = 300;   // ms

bool timerStarted = false;

const unsigned long time_LED = 8000;   // 8 seconds per LED
const unsigned long time_out = 1000;   // keep all LEDs OFF for 1s if user fails

// Interrupt flags
volatile bool resetGamePressed = false;
volatile bool skipPressed = false;

// States
enum Phase { RUNNING, FAIL };
Phase phase = RUNNING;

unsigned long phaseStartMs = 0;   // start time of current phase

void setTimerLEDs(bool on) {
  if (on) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
  } else {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
  }
}

void setLivesLEDs(bool on)
{
  if (on) {
    digitalWrite(LIFE1, HIGH);
    digitalWrite(LIFE2, HIGH);
    digitalWrite(LIFE3, HIGH);
  } else {
    digitalWrite(LIFE1, LOW);
    digitalWrite(LIFE2, LOW);
    digitalWrite(LIFE3, LOW);
  }
}

void setPointLEDs(bool on)
{
  if (on) {
    digitalWrite(POINT1, HIGH);
    digitalWrite(POINT2, HIGH);
    digitalWrite(POINT3, HIGH);
  } else {
    digitalWrite(POINT1, LOW);
    digitalWrite(POINT2, LOW);
    digitalWrite(POINT3, LOW);
  }
}

void resetGameTimer() {
  phase = RUNNING;
  phaseStartMs = millis();
  setTimerLEDs(true);
}

// ISR for pin 2
void resetGameISR() {
  resetGamePressed = true;
}

// ISR for pin 3
void skipISR() {
  skipPressed = true;
}

void gameOver()
{
  gameOverActive = true;
  flashState = true;
  phase = FAIL;
  resetGameTimer();
  setTimerLEDs(false);
  setLivesLEDs(true);
  setPointLEDs(true);
}

void handleGameOverFlash() {
  unsigned long now = millis();

  if (now - lastFlashMs >= flashInterval) {
    lastFlashMs = now;
    flashState = !flashState;
    setTimerLEDs(flashState);
    setLivesLEDs(flashState);
    setPointLEDs(flashState);
  }
}

void updateLives() 
{
  if (lifeCount >= 3) {
    digitalWrite(LIFE1, HIGH);
    digitalWrite(LIFE2, HIGH);
    digitalWrite(LIFE3, HIGH);
  }

  else if (lifeCount == 2) {
    digitalWrite(LIFE1, LOW);
    digitalWrite(LIFE2, HIGH);
    digitalWrite(LIFE3, HIGH);
  }

  else if (lifeCount == 1) {
    digitalWrite(LIFE1, LOW);
    digitalWrite(LIFE2, LOW);
    digitalWrite(LIFE3, HIGH);
  }

  else {
    gameOver();
  }
}

void updatePoints()
{
  if (pointcount >= 3) {
    digitalWrite(POINT1, HIGH);
    digitalWrite(POINT2, HIGH);
    digitalWrite(POINT3, HIGH);
  }
  else if (pointcount == 2) {
    digitalWrite(POINT1, HIGH);
    digitalWrite(POINT2, HIGH);
    digitalWrite(POINT3, LOW);
  }
  else if (pointcount == 1) {
    digitalWrite(POINT1, HIGH);
    digitalWrite(POINT2, LOW);
    digitalWrite(POINT3, LOW);
  }
  else {
    digitalWrite(POINT1, LOW);
    digitalWrite(POINT2, LOW);
    digitalWrite(POINT3, LOW);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  // pin 13 button to GND
  pinMode(morse, INPUT);

  // interrupt buttons
  // use INPUT if button goes to GND
  pinMode(reset_game, INPUT);
  pinMode(skip, INPUT);

  attachInterrupt(digitalPinToInterrupt(reset_game), resetGameISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(skip), skipISR, FALLING);

  // do NOT start timer yet
  setTimerLEDs(false);

  //lifesetup
  pinMode(LIFE1, OUTPUT);
  pinMode(LIFE2, OUTPUT);
  pinMode(LIFE3, OUTPUT);

  //point setup
  pinMode(POINT1, OUTPUT);
  pinMode(POINT2, OUTPUT);
  pinMode(POINT3, OUTPUT);

  updateLives();
  updatePoints();
}

void loop() {
  unsigned long now = millis();

  if (gameOverActive) {
    handleGameOverFlash();

    // only reset button exits game over
    if (resetGamePressed) {
      resetGamePressed = false;

      gameOverActive = false;
      flashState = false;

      // reset lives / points here
      lifeCount = 3;
      pointcount = 0;

      updateLives();
      updatePoints();

      setTimerLEDs(false);   // countdown LEDs off until game restarts
    }

    return;
  }

  // Start timer only when pin 13 button is pressed
  if (!timerStarted) {
    if (digitalRead(morse) == LOW) {   // button pressed
      Serial.println("Morse button pressed: timer started");
      timerStarted = true;
      resetGameTimer();

      // wait for release so one press only starts once
      while (digitalRead(morse) == LOW) {
      }
    }
    return;
  }

  // Pin 2 pressed: reset game and timer
  if (resetGamePressed) 
  {
    resetGamePressed = false;
    // later: reset score, lives, etc. here
    Serial.println("Reset game pressed");
    lifeCount = 3;
    pointcount = 0;
    updateLives();
    updatePoints();
    resetGameTimer();
    return;
  }

  // Pin 3 pressed: skip and reset timer
  if (skipPressed) 
  {
    skipPressed = false;

    // later: skip current word here
    Serial.println("Skip pressed");

    resetGameTimer();
    return;
  }

  if (phase == RUNNING) 
  {
    unsigned long elapsed = now - phaseStartMs;

    // LED countdown logic
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
      // 16-24s: LED1 and LED2 off
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, HIGH);
    } 
    else {
      // 24s reached
      setTimerLEDs(false);
      // delay(1000);
      phase = FAIL; //subsequently lose life here
      lifeCount--;
      updateLives();
      Serial.print(lifeCount);
      phaseStartMs = now; //resets timer
    }
  } 
  else if (phase == FAIL) 
  {
    // All LEDs OFF for 2s, then restart timer
    if (now - phaseStartMs >= time_out) {
      resetGameTimer();
      return;
    }
  }
}
