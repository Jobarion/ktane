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

struct Maze {
  uint8_t target;
  uint8_t circle1;
  uint8_t circle2;
  Transition* walls;
  uint8_t wallCount;
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
Maze maze;

void initGame() {
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  randomSeed(analogRead(0));
  maze = getMaze(random(0, 9));
}

void startGame() {
  pixels.begin();
  pixels.setPixelColor(maze.circle1, pixels.Color(20, 5, 0));
  pixels.setPixelColor(maze.circle2, pixels.Color(20, 5, 0));
  pixels.show();
  Serial.println(maze.wallCount);
  Serial.println(maze.walls[0].p1);
}

void gameLoop() {
  Direction d = readDirection();
  uint8_t pos = currentPos;
  switch(d) {
    case NONE: {
      return;
    }
    case UP: {
      pos += 6;
      break;
    }
    case DOWN: {
      pos -= 6;
      break;
    }
    case RIGHT: {
      pos++;
      break;
    }
    case LEFT: {
      pos--;
      break;
    }
  }
  bool allowed = isAllowed(currentPos, pos);
  if(!allowed) {
    strike();
    return;
  }
  pixels.setPixelColor(currentPos, pixels.Color(0, 0, 0));
  pixels.setPixelColor(maze.circle1, pixels.Color(20, 5, 0));
  pixels.setPixelColor(maze.circle2, pixels.Color(20, 5, 0));
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
  for(int i = 0; i < maze.wallCount; i++) {
    if(maze.walls[i].p1 == oldPos && maze.walls[i].p2 == newPos) {
      return false;
    }
    else if(maze.walls[i].p2 == oldPos && maze.walls[i].p1 == newPos) {
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

Maze getMaze(uint8_t mazeId) {
  mazeId = 0;
  Maze maze;
  switch(mazeId) {
    case 0: {
      maze.circle1 = 24;
      maze.circle2 = 23;
      maze.wallCount = 25;
      maze.walls[0] = {1,2};
      maze.walls[1] = {3,4};
      maze.walls[2] = {8,9};
      maze.walls[3] = {10,11};
      maze.walls[4] = {12,13};
      maze.walls[5] = {15,16};
      maze.walls[6] = {18,19};
      maze.walls[7] = {20,21};
      maze.walls[8] = {24,25};
      maze.walls[9] = {26,27};
      maze.walls[10] = {32,33};
      maze.walls[11] = {1,7};
      maze.walls[12] = {4,10};
      maze.walls[13] = {7,13};
      maze.walls[14] = {13,19};
      maze.walls[15] = {25,31};
      maze.walls[16] = {5,11};
      maze.walls[17] = {14,20};
      maze.walls[18] = {6,12};
      maze.walls[19] = {15,21};
      maze.walls[20] = {4,10};
      maze.walls[21] = {10,16};
      maze.walls[22] = {16,22};
      maze.walls[23] = {22,28};
      maze.walls[24] = {28,34};
      break;
    }
  }
  return maze;
}
