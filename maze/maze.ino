#include <Ktane.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
 
#define PIN       2
#define PIN_UP    3
#define PIN_RIGHT 4
#define PIN_DOWN  5
#define PIN_LEFT  6
#define NUMPIXELS 36
 
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

enum Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  NONE
};

struct Transition {
  uint8_t p1;
  uint8_t p2;  
};

Network* network;
Ktane* ktane;

void setup() {
  network = new Network(9600);
  //network->init(false);
  ktane = new Ktane(network, *initGame, *startGame, *gameLoop, NULL);
  initGame();
  startGame();
}

void loop() {
  ktane->refresh();
  gameLoop();
}

bool acceptInput = false;
uint8_t currentPos = 3 + 6 * 3;
Transition walls[1];

void initGame() {
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  walls[0] = {8,9};
}

void startGame() {
  pixels.begin();
}

void gameLoop() {
  Direction d = readDirection();
  uint8_t pos = currentPos;
  switch(d) {
    case UP: {
      if(pos <= 30) {
        pos += 6;
      }
      break;
    }
    case DOWN: {
      if(pos >= 6) {
        pos -= 6;
      }
      break;
    }
    case RIGHT: {
      if(pos % 6 < 5) {
        pos++;
      }
      break;
    }
    case LEFT: {
      if(pos % 6 > 0) {
        pos--;
      }
      break;
    }
  }
  bool allowed = isAllowed(currentPos, pos);
  if(!allowed) {
    strike();
    return;
  }
  pixels.setPixelColor(currentPos, pixels.Color(0, 0, 0));
  pixels.setPixelColor(pos, pixels.Color(50, 0, 0));
  pixels.show();
  currentPos = pos;
}

Direction readDirection() {
  Direction d = NONE;
  if(digitalRead(PIN_UP) == LOW) {
    d = UP;
  }
  else if(digitalRead(PIN_DOWN) == LOW) {
    d = DOWN;
  }
  else if(digitalRead(PIN_LEFT) == LOW) {
    d = LEFT;
  }
  else if(digitalRead(PIN_RIGHT) == LOW) {
    d = RIGHT;
  }
  if(acceptInput) {
    if(d != NONE) {
      acceptInput = false;
    }
    return d;
  }
  else {
    if(d == NONE) {
      acceptInput = true;
    }
    return NONE;
  }
}

bool isAllowed(uint8_t oldPos, uint8_t newPos) {
  for(int i = 0; i < 1; i++) {
    if(walls[i].p1 == oldPos && walls[i].p2 == newPos) {
      return false;
    }
    else if(walls[i].p2 == oldPos && walls[i].p1 == newPos) {
      return false;
    }
  }
  if(oldPos + 1 == newPos) {
    return oldPos % 6 < 5;
  }
  else if(oldPos - 1 == newPos) {
    return oldPos % 6 > 0;
  }
  else if(oldPos + 6 == newPos) {
    return newPos <= 35;
  }
  else if(oldPos - 6 == newPos) {
    return newPos >= 0;
  }
  return true;
}

void strike() {
  
}
