#include <Ktane.h>
#include <Encoder.h>
#include <Bounce2.h>
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <limits.h>

//Display
#define D_1 14
#define SEG_A 4
#define TIMER_10MSEC 65473
#define FREQUENCY_START 3500

//Morse code
#define DURATION_SHORT 200
#define DURATION_LONG (DURATION_SHORT * 3)
#define WORD_COUNT 16
#define MORSE_PIN 18
#define DONE_PIN 12
#define TX_PIN 11


typedef struct {
  char word[7];
  int khz;
} word_t;

enum morse_symbol {
  SHORT,
  LONG,
  BREAK,
  EMPTY
};

const word_t words[WORD_COUNT] PROGMEM = {
   {"shell", 3505},
   {"halls", 3515},
   {"slick", 3522},
   {"trick", 3532},
   {"boxes", 3535},
   {"leaks", 3542},
   {"strobe",3545},
   {"bistro",3552},
   {"flick", 3555},
   {"bombs", 3565},
   {"break", 3572},
   {"brick", 3575},
   {"steak", 3582},
   {"sting", 3592},
   {"vector",3595},
   {"beats", 3600}
};

const morse_symbol morseTable[26][5] = {
  {SHORT, LONG, BREAK, EMPTY, EMPTY}, //A
  {LONG, SHORT, SHORT, SHORT, BREAK}, //B
  {LONG, SHORT, LONG, SHORT, BREAK},  //C
  {LONG, SHORT, SHORT, BREAK, EMPTY}, //D
  {SHORT, BREAK, EMPTY, EMPTY, EMPTY},//E
  {SHORT, SHORT, LONG, SHORT, BREAK}, //F
  {LONG, LONG, SHORT, BREAK, EMPTY},  //G
  {SHORT, SHORT, SHORT, SHORT, BREAK},//H
  {SHORT, SHORT, BREAK, EMPTY, EMPTY},//I
  {SHORT, LONG, LONG, LONG, BREAK},   //J
  {LONG, SHORT, LONG, BREAK, EMPTY},  //K
  {SHORT, LONG, SHORT, SHORT, BREAK}, //L
  {LONG, LONG, BREAK, EMPTY, EMPTY},  //M
  {LONG, SHORT, BREAK, EMPTY, EMPTY}, //N
  {LONG, LONG, LONG, BREAK, EMPTY},   //O
  {SHORT, LONG, LONG, SHORT, BREAK},  //P
  {LONG, LONG, SHORT, LONG, BREAK},   //Q
  {SHORT, LONG, SHORT, BREAK, EMPTY}, //R
  {SHORT, SHORT, SHORT, BREAK, EMPTY},//S
  {LONG, BREAK, EMPTY, EMPTY, EMPTY}, //T
  {SHORT, SHORT, LONG, BREAK, EMPTY}, //U
  {SHORT, SHORT, SHORT, LONG, BREAK}, //V
  {SHORT, LONG, LONG, BREAK, EMPTY},  //W
  {LONG, SHORT, SHORT, LONG, BREAK},  //X
  {LONG, SHORT, LONG, LONG, BREAK},   //Y
  {LONG, LONG, SHORT, SHORT, BREAK}   //Z
};

Network* network;
Ktane* ktane;

void setup() {
  network = new Network(9600);
  network->init(false);
  ktane = new Ktane(network, *initGame, *startGame, *gameLoop, *cleanupGame, NULL);
  ktane->KTANE_MODULE_ID = 6;
}

void loop() {
  ktane->refresh();
}

void completed() {
  ktane->gameState.phase = OVER;
  uint8_t buffer[1];
  buffer[0] = 6;
  network->send(0, buffer, 1);
  digitalWrite(MORSE_PIN, LOW);
  digitalWrite(DONE_PIN, HIGH);
}

void strike() {
  uint8_t buffer[] = {4};
  network->broadcast(buffer, 1);
  ktane->gameState.strikes++;
}

// MORSE CODE //

word_t target;
int wordPosition = 0;
int charPosition = 0;
int durationLeft = 0;
long lastTime;
bool pressed = false;
Bounce txButton;

Encoder knob(2, 3);
long frequency = FREQUENCY_START;

void cleanupGame() {
  digitalWrite(MORSE_PIN, LOW);
}

void initGame() {
  //MORSE CODE
  randomSeed(analogRead(5));
  memcpy_P( &target, &words[random(WORD_COUNT)], sizeof(word_t));
  pinMode(MORSE_PIN, OUTPUT);
  digitalWrite(MORSE_PIN, LOW);

  //DISPLAY
  calculateDigits();
  for(int i = SEG_A; i < SEG_A + 7; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }
  for(int i = 0; i < 4; i++) {
    pinMode(D_1 + i, OUTPUT);
    digitalWrite(D_1 + i, LOW);
  }
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = TIMER_10MSEC;
  TCCR1B |= (1 << CS12);
  interrupts();

  pinMode(MORSE_PIN, OUTPUT);
  digitalWrite(MORSE_PIN, LOW);
  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  pinMode(TX_PIN, INPUT_PULLUP);
  txButton = Bounce();
  txButton.attach(TX_PIN, INPUT_PULLUP);
  txButton.interval(50);
  
  pinMode(TX_PIN, INPUT_PULLUP);
}

void startGame() {
  lastTime = millis();
  TIMSK1 |= (1 << TOIE1);//Start timer
}

void gameLoop() {
  long newFrequency = FREQUENCY_START + knob.read() / 4;
  if(newFrequency != frequency) {
    frequency = newFrequency;
    calculateDigits();
  }
  
  txButton.update();
  if(txButton.read() == HIGH) {
    pressed = false;
  }
  else if(!pressed) {
    pressed = true;
    if(frequency == target.khz) {
      completed();
    }
    else {
      strike();
    }
  }
  
  durationLeft -= deltaT();
  if(durationLeft <= 0) {
    advance();
  }
}

long deltaT() {
  long curTime = millis();
  long deltaT = curTime - lastTime;
  lastTime = curTime;
  if(deltaT < 0) {
    deltaT += INT_MAX;
  }
  return deltaT;
}

void advance() {
  if(digitalRead(MORSE_PIN)) {
    charPosition++;
    if(currentSymbol() == BREAK) {
      wordPosition++;
      charPosition = 0;
      if(target.word[wordPosition] == '\0') {
        wordPosition = 0;
        durationLeft = DURATION_SHORT * 7; //Word break
      }
      else {
        durationLeft = DURATION_SHORT * 3; //Character break
      }
    }
    else {
      durationLeft = DURATION_SHORT; //Morse symbol break
    }
    digitalWrite(MORSE_PIN, LOW);
    return;
  }
  digitalWrite(MORSE_PIN, HIGH);
  switch(currentSymbol()) {
    case SHORT: {
      durationLeft = DURATION_SHORT;
      break;
    }
    case LONG: {
      durationLeft = DURATION_LONG;
      break;
    }
  }
}

morse_symbol currentSymbol() {
  char c = target.word[wordPosition];
  const morse_symbol* code = morseTable[c - 'a'];
  return code[charPosition];  
}

// DISPLAY //

struct LED_PINS {
  int16_t D;
  int16_t B;
};

int currentPin = D_1;
LED_PINS digitPins[4];

ISR(TIMER1_OVF_vect) {
  currentPin = (currentPin == D_1 + 3 ? D_1 : currentPin + 1);
  PORTD = PORTD | B11110000;
  PORTD = PORTD & digitPins[currentPin - D_1].D;
  PORTB = PORTB | B00000111;
  PORTB = PORTB & digitPins[currentPin - D_1].B;
  PORTC = PORTC & B11110000;
  PORTC = PORTC | (1 << (currentPin - D_1));
  TCNT1 = TIMER_10MSEC;
}

void calculateDigits() {
  uint8_t digits[4];
  digits[3] = (frequency / 1000) % 10;
  digits[2] = (frequency / 100) % 10;
  digits[1] = (frequency / 10) % 10;
  digits[0] = frequency % 10;
  for(int i = 0; i < 4; i++) {
    digitPins[i] = getPinData(digits[i]);
  }
}

struct LED_PINS getPinData(int digit) {
  LED_PINS pins;
  switch(digit) {
    case 0: {
      pins.D = ~B11110000;
      pins.B = ~B00000011;
      break;
    }
    case 1: {
      pins.D = ~B01100000;
      pins.B = ~B00000000;
      break;
    }
    case 2: {
      pins.D = ~B10110000;
      pins.B = ~B00000101;
      break;
    }
    case 3: {
      pins.D = ~B11110000;
      pins.B = ~B00000100;
      break;
    }
    case 4: {
      pins.D = ~B01100000;
      pins.B = ~B00000110;
      break;
    }
    case 5: {
      pins.D = ~B11010000;
      pins.B = ~B00000110;
      break;
    }
    case 6: {
      pins.D = ~B11010000;
      pins.B = ~B00000111;
      break;
    }
    case 7: {
      pins.D = ~B01110000;
      pins.B = ~B00000000;
      break;
    }
    case 8: {
      pins.D = ~B11110000;
      pins.B = ~B00000111;
      break;
    }
    case 9: {
      pins.D = ~B11110000;
      pins.B = ~B00000110;
      break;
    }
  }
  return pins;
}
