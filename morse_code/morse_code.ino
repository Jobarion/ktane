#include <Arduino.h>
#include <avr/pgmspace.h>
#include <limits.h>

#define DURATION_LONG 600
#define DURATION_SHORT 200
#define DURATION_BREAK 1000
#define CHAR_PAUSE 100
#define WORD_PAUSE 2000
#define WORD_COUNT 16
#define MORSE_PIN LED_BUILTIN

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

word_t target;
uint8_t wordPosition = -1;
uint8_t charPosition = 0;
int durationLeft = 0;
long lastTime;

void setup() {
  randomSeed(analogRead(0));
  memcpy_P( &target, &words[random(WORD_COUNT)], sizeof(word_t));
  pinMode(MORSE_PIN, OUTPUT);
  digitalWrite(MORSE_PIN, LOW);
  lastTime = millis();
  delay(2000);
}

void loop() {
  long curTime = millis();
  long deltaT = curTime - lastTime;
  lastTime = curTime;
  if(deltaT < 0) {
    deltaT += INT_MAX;
  }
  durationLeft -= deltaT;
  if(durationLeft <= 0) {
    advance();
  }
}

void advance() {
  if(wordPosition >= 0 && digitalRead(MORSE_PIN)) {
    durationLeft = CHAR_PAUSE;
    digitalWrite(MORSE_PIN, LOW);
    return;
  }
  if(wordPosition < 0) {
    wordPosition = 0;
  }
    if(currentSymbol() == BREAK) {
      wordPosition++;
      charPosition = 0;
      if(target.word[wordPosition] == '\0') {
        wordPosition = 0;
        durationLeft = WORD_PAUSE;
        return;
      }
    }
    else {
      charPosition++;
    }
    switch(currentSymbol()) {
      case SHORT: {
        digitalWrite(MORSE_PIN, HIGH);
        durationLeft = DURATION_SHORT;
        break;
      }
      case LONG: {
        digitalWrite(MORSE_PIN, HIGH);
        durationLeft = DURATION_LONG;
        break;
      }
      case BREAK: {
        durationLeft = DURATION_BREAK;
        break;
      }
    }
}

morse_symbol currentSymbol() {
  char c = target.word[wordPosition];
  const morse_symbol* code = morseTable[c - 'a'];
  return code[charPosition];  
}
