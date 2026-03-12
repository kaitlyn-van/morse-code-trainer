# 📡 Morse Code Trainer

An Arduino-based interactive game that teaches players to input Morse code using a physical button. Players progress through three difficulty levels by correctly spelling words in Morse code within a 24-second time limit. ⏱️

## 🎮 How to Play

1. Wire up the circuit (see pin configuration below)
2. Upload `morse_trainer.ino` to your Arduino Uno
3. Open Serial Monitor at 115200 baud
4. Press the morse button to start the game 🟢
5. A target word will appear in the Serial Monitor
6. Input each letter using dots (short press) and dashes (long press)
7. Pause for 800ms between letters to commit them
8. Get 3 words correct to advance to the next level 🏆
9. You have 3 lives — lose one for each wrong word, skip, or timeout 💀

## 📶 Difficulty Levels

| Level | Words to Advance |
|-------|-----------------|
| 🟢 Beginner | 3 |
| 🟡 Medium | 3 |
| 🔴 Hard | 3 |

## 📌 Pin Configuration

### ⏱️ Timer LEDs (Blue)
| Pin | Function |
|-----|----------|
| 4 | LED1 - turns off at 8s |
| 5 | LED2 - turns off at 16s |
| 6 | LED3 - turns off at 24s |

### ❤️ Lives LEDs (Red)
| Pin | Function |
|-----|----------|
| 11 | LIFE1 |
| 12 | LIFE2 |
| A0 | LIFE3 |

### 🔘 Buttons
| Pin | Function |
|-----|----------|
| 3 | Morse input (polled) |
| 2 | OR-gate output of all 4 game buttons (INT0) |
| 7 | Next level ⏩ |
| 8 | Back level ⏪ |
| 9 | Reset game 🔄 |
| 10 | Skip word (costs a life) ⚠️ |

## 🔧 Hardware Requirements

- Arduino Uno
- 6x LEDs with resistors (3 blue, 3 red)
- 5x push buttons
- 1x OR gate chip (74HC32) to combine game buttons into pin 2
- Pull-down resistors (10kΩ) on each button input

## 📁 File Structure
```
morse_trainer/
├── morse_trainer.ino    # main game logic
├── morse_lookup.h       # morse code character lookup declarations
├── morse_lookup.cpp     # morse code character lookup implementation
└── word_bank.h          # beginner, medium, and hard word banks
```

## ⚙️ Technical Design

### ⏲️ Timer
- Uses Timer2 in CTC mode at 1ms intervals to replace `millis()`
- `tickCount` is the single time source for all timed events
- No Arduino time functions (`millis()`, `delay()`, `tone()`) are used anywhere

### 📊 Timed Events (all derived from tickCount)
- Dot vs dash classification (200ms threshold)
- Inter-letter gap detection (800ms silence)
- 24-second word countdown
- Message display hold (2 seconds)
- Fail state pause (1 second)

### ⚡ Interrupts
- All 4 game buttons are OR-gated into a single interrupt pin (pin 2 / INT0)
- `gameButtonISR()` reads individual sense pins to identify which button fired
- Timer2 ISR increments `tickCount` every 1ms

## 👩‍💻 Authours
Kaitlyn Van and Natasha Poon
