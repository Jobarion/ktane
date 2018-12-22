#include <Ktane.h>

#define SECONDS 720
#define D_1 14
#define SEG_A 2
#define TIMER_10MSEC 65473
//64911

long hundSecs = 7200;
long cTime = hundSecs * 10;
Network* network;
Ktane* ktane;

void setup() {
  network = new Network(9600);
  network->init(false);
  ktane = new Ktane(network, *initGame, *startGame, *gameLoop, NULL);
}

void loop() {
  ktane->refresh();
}

struct LED_PINS {
  uint8_t D;
  uint8_t B;
};

int currentPin = D_1;
LED_PINS digitPins[4];

void initGame() {
  for(int i = SEG_A; i < SEG_A + 7; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  for(int i = 0; i < 4; i++) {
    digitalWrite(D_1 + i, HIGH);
    pinMode(D_1 + i, OUTPUT);
  }
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = TIMER_10MSEC;
  TCCR1B |= (1 << CS12);
  interrupts();
}

void startGame() {
  calculateDigits();
  TIMSK1 |= (1 << TOIE1);
}

void gameLoop() {
  if(hundSecs <= 0) {
    terminate();
    return;
  }
  calculateDigits();
}

void terminate() {
  ktane->gameState.phase = OVER;
  uint8_t buffer[] = {3};
  network->broadcast(buffer, 1);
}

ISR(TIMER1_OVF_vect) {
  cTime--;
  hundSecs = cTime / 10;
  currentPin = (currentPin == D_1 + 3 ? D_1 : currentPin + 1);
  PORTD = PORTD & B00000011;
  PORTD = PORTD | digitPins[currentPin - D_1].D;
  PORTB = PORTB & B11111110;
  PORTB = PORTB | digitPins[currentPin - D_1].B;
  PORTC = PORTC & B11110000;
  PORTC = PORTC | ((1 << (currentPin - D_1)) ^ B00001111);
  TCNT1 = TIMER_10MSEC;
}

void calculateDigits() {
  uint8_t digits[4];
  if(hundSecs >= 6000) {
    int secs = hundSecs / 100;
    digits[0] = secs / 600;
    digits[1] = (secs / 60) % 10;
    digits[2] = (secs % 60) / 10;
    digits[3] = secs % 10;
  }
  else {
    digits[0] = (hundSecs / 1000) % 10;
    digits[1] = (hundSecs / 100) % 10;
    digits[2] = (hundSecs / 10) % 10;
    digits[3] = hundSecs % 10;
  }
  for(int i = 0; i < 4; i++) {
    digitPins[i] = getPinData(digits[i]);
  }
}

struct LED_PINS getPinData(int digit) {
  LED_PINS pins;
  switch(digit) {
    case 0: {
      pins.D = B11111100;
      pins.B = B00000000;
      break;
    }
    case 1: {
      pins.D = B00011000;
      pins.B = B11111110;
      break;
    }
    case 2: {
      pins.D = B01101100;
      pins.B = B00000001;
      break;
    }
    case 3: {
      pins.D = B00111100;
      pins.B = B00000001;
      break;
    }
    case 4: {
      pins.D = B10011000;
      pins.B = B00000001;
      break;
    }
    case 5: {
      pins.D = B10110100;
      pins.B = B00000001;
      break;
    }
    case 6: {
      pins.D = B11110100;
      pins.B = B00000001;
      break;
    }
    case 7: {
      pins.D = B00011100;
      pins.B = B11111110;
      break;
    }
    case 8: {
      pins.D = B11111100;
      pins.B = B00000001;
      break;
    }
    case 9: {
      pins.D = B10111100;
      pins.B = B00000001;
      break;
    }
  }
  return pins;
}

void clearPins() {
  PORTD = PORTD & B00000011;
  PORTB = PORTB & B11111110;
}
