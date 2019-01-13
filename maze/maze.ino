#include <Ktane.h>
#include <Bounce2.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <limits.h>
 
#define PIN       2
#define PIN_UP    3
#define PIN_RIGHT 4
#define PIN_DOWN  5
#define PIN_LEFT  6
#define PIN_SOLVED 7
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
  uint8_t goal = 100;
  Transition* walls;
  uint8_t wallCount;
};

Network* network;
Ktane* ktane;
Bounce buttons[4];//UDLR
bool solved = false;
long strikeDuration = 0;
long lastTime;

void setup() {
  network = new Network(9600);
  network->init(false);
  ktane = new Ktane(network, *initGame, *startGame, *gameLoop, *acceptPacket);
}

void loop() {
  ktane->refresh();
}

bool acceptInput = false;
uint8_t currentPos = 100;
Maze maze;

void initGame() {
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  randomSeed(analogRead(0) ^ analogRead(1) ^ analogRead(2));
  maze = getMaze(random(0, 9));
  buttons[0] = Bounce();
  buttons[0].attach(PIN_UP);
  buttons[0].interval(50);
  buttons[1] = Bounce();
  buttons[1].attach(PIN_DOWN);
  buttons[1].interval(50);
  buttons[2] = Bounce();
  buttons[2].attach(PIN_LEFT);
  buttons[2].interval(50);
  buttons[3] = Bounce();
  buttons[3].attach(PIN_RIGHT);
  buttons[3].interval(50);
  digitalWrite(PIN_SOLVED, LOW);
  pinMode(PIN_SOLVED, OUTPUT);
  while(currentPos == 100) {
    uint8_t start = random(0, 36);
    if(start != maze.circle1 && start != maze.circle2) {
      currentPos = start;
    }
  }
  while(maze.goal == 100) {
    uint8_t goal = random(0, 36);
    if(goal != maze.circle1 && goal != maze.circle2 && goal != currentPos) {
      maze.goal = goal;
    }
  }
}

void startGame() {
  lastTime = millis();
  pixels.begin();
  writeLeds();
}

void gameLoop() {
  if(solved) {
    digitalWrite(PIN_SOLVED, HIGH);
    return;
  }
  if(strikeDuration > 0) {
    strikeDuration -= deltaT();
    if(strikeDuration <= 0) {
      strikeDuration = 0;
      writeLeds();
    }
    return;
  }
  for(int i = 0; i < 4; i++) {
    buttons[i].update();
  }
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
  currentPos = pos;
  writeLeds();
  if(currentPos == maze.goal) {
    pixels.setPixelColor(currentPos, pixels.Color(0, 0, 0));
    pixels.setPixelColor(maze.circle1, pixels.Color(0, 0, 0));
    pixels.setPixelColor(maze.circle2, pixels.Color(0, 0, 0));
    pixels.show();
    solved = true;
  }
}

void writeLeds() {
  pixels.setPixelColor(maze.circle1, pixels.Color(0, 0, 20));
  pixels.setPixelColor(maze.circle2, pixels.Color(0, 0, 20));
  pixels.setPixelColor(maze.goal, pixels.Color(40, 0, 0));
  pixels.setPixelColor(currentPos, pixels.Color(25, 25, 25));
  pixels.show();
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

Direction readDirection() {
  Direction d = NONE;
  if(buttons[0].read() == LOW) {
    d = UP;
  }
  else if(buttons[1].read() == LOW) {
    d = DOWN;
  }
  else if(buttons[2].read() == LOW) {
    d = LEFT;
  }
  else if(buttons[3].read() == LOW) {
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
  else if(oldPos == newPos + 6) {
    return oldPos >= 6;
  }
  return false;
}

void strike() {
  pixels.setPixelColor(currentPos, pixels.Color(80, 0, 0));
  pixels.setPixelColor(maze.circle1, pixels.Color(80, 0, 0));
  pixels.setPixelColor(maze.circle2, pixels.Color(80, 0, 0));
  pixels.setPixelColor(maze.goal, pixels.Color(80, 0, 0));
  pixels.show();
  strikeDuration = 2000;
  uint8_t buffer[] = {4};
  network->broadcast(buffer, 1);
  deltaT();//Reset delta timer
}

void acceptPacket(Network* network, const uint8_t src, const uint8_t *buffer, const size_t length) {
  if(length == 0 || buffer == NULL) return;
  switch(buffer[0]) {
    case 3: {
      pixels.setPixelColor(currentPos, pixels.Color(0, 0, 0));
      pixels.setPixelColor(maze.circle1, pixels.Color(0, 0, 0));
      pixels.setPixelColor(maze.circle2, pixels.Color(0, 0, 0));
      pixels.setPixelColor(maze.goal, pixels.Color(0, 0, 0));
      pixels.show();
    }
  }
}

Maze getMaze(uint8_t mazeId) {
  Maze maze;
  switch(mazeId) {
    case 0: {
      maze.circle1 = 24;
      maze.circle2 = 23;
      maze.wallCount = 25;
      maze.walls = new Transition[maze.wallCount];
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
      maze.walls[12] = {7,13};
      maze.walls[13] = {13,19};
      maze.walls[14] = {25,31};
      maze.walls[15] = {4,10};
      maze.walls[16] = {10,16};
      maze.walls[17] = {16,22};
      maze.walls[18] = {22,28};
      maze.walls[19] = {28,34};
      maze.walls[20] = {9,15};
      maze.walls[21] = {8,14};
      maze.walls[22] = {21,27};
      maze.walls[23] = {20,26};
      maze.walls[24] = {29,35};
      break;
    }
    case 1: {
      maze.circle1 = 13;
      maze.circle2 = 28;
      maze.wallCount = 25;
      maze.walls = new Transition[maze.wallCount];
      maze.walls[0] = {24,30};
      maze.walls[1] = {7,13};
      maze.walls[2] = {19,25};
      maze.walls[3] = {14,20};
      maze.walls[4] = {26,32};
      maze.walls[5] = {9,15};
      maze.walls[6] = {21,27};
      maze.walls[7] = {4,10};
      maze.walls[8] = {16,22};
      maze.walls[9] = {22,28};
      maze.walls[10] = {29,35};
      maze.walls[11] = {0,1};
      maze.walls[12] = {2,3};
      maze.walls[13] = {6,7};
      maze.walls[14] = {7,8};
      maze.walls[15] = {8,9};
      maze.walls[16] = {10,11};
      maze.walls[17] = {13,14};
      maze.walls[18] = {15,16};
      maze.walls[19] = {16,17};
      maze.walls[20] = {18,19};
      maze.walls[21] = {20,21};
      maze.walls[22] = {25,26};
      maze.walls[23] = {27,28};
      maze.walls[24] = {32,33};
      break;
    }    
    case 2: {
      maze.circle1 = 15;
      maze.circle2 = 17;
      maze.wallCount = 25;
      maze.walls = new Transition[maze.wallCount];
      maze.walls[0] = {18,24};
      maze.walls[1] = {1,7};
      maze.walls[2] = {25,31};
      maze.walls[3] = {2,8};
      maze.walls[4] = {21,27};
      maze.walls[5] = {22,28};
      maze.walls[6] = {3,4};
      maze.walls[7] = {6,7};
      maze.walls[8] = {8,9};
      maze.walls[9] = {9,10};
      maze.walls[24] = {10,11};
      maze.walls[10] = {12,13};
      maze.walls[11] = {13,14};
      maze.walls[12] = {14,15};
      maze.walls[13] = {15,16};
      maze.walls[14] = {16,17};
      maze.walls[15] = {19,20};
      maze.walls[16] = {20,21};
      maze.walls[17] = {22,23};
      maze.walls[18] = {24,25};
      maze.walls[19] = {25,26};
      maze.walls[20] = {26,27};
      maze.walls[21] = {28,29};
      maze.walls[22] = {32,33};
      maze.walls[23] = {33,34};
      break;
    }
    case 3: {
      maze.circle1 = 12;
      maze.circle2 = 30;
      maze.wallCount = 25;
      maze.walls = new Transition[maze.wallCount];
      maze.walls[0] = {2,3};
      maze.walls[1] = {4,5};
      maze.walls[2] = {10,11};
      maze.walls[3] = {12,13};
      maze.walls[4] = {18,19};
      maze.walls[5] = {20,21};
      maze.walls[6] = {22,23};
      maze.walls[7] = {24,25};
      maze.walls[8] = {25,26};
      maze.walls[9] = {31,32};
      maze.walls[24] = {1,7};
      maze.walls[10] = {7,13};
      maze.walls[11] = {13,19};
      maze.walls[12] = {2,8};
      maze.walls[13] = {8,14};
      maze.walls[14] = {14,20};
      maze.walls[15] = {26,32};
      maze.walls[16] = {3,9};
      maze.walls[17] = {9,15};
      maze.walls[18] = {21,27};
      maze.walls[19] = {27,33};
      maze.walls[20] = {10,16};
      maze.walls[21] = {16,22};
      maze.walls[22] = {22,28};
      maze.walls[23] = {28,34};
      break;
    }
    case 4: {
      maze.circle1 = 3;
      maze.circle2 = 22;
      maze.wallCount = 25;
      maze.walls = new Transition[maze.wallCount];
      maze.walls[0] = {0,1};
      maze.walls[1] = {6,7};
      maze.walls[2] = {10,11};
      maze.walls[3] = {12,13};
      maze.walls[4] = {15,16};
      maze.walls[5] = {16,17};
      maze.walls[6] = {19,20};
      maze.walls[7] = {21,22};
      maze.walls[8] = {28,29};
      maze.walls[9] = {24,30};
      maze.walls[24] = {7,13};
      maze.walls[10] = {19,25};
      maze.walls[11] = {25,31};
      maze.walls[12] = {2,8};
      maze.walls[13] = {8,14};
      maze.walls[14] = {14,20};
      maze.walls[15] = {20,26};
      maze.walls[16] = {26,32};
      maze.walls[17] = {3,9};
      maze.walls[18] = {15,21};
      maze.walls[19] = {27,33};
      maze.walls[20] = {4,10};
      maze.walls[21] = {10,16};
      maze.walls[22] = {22,28};
      maze.walls[23] = {23,29};
      break;
    }
    case 5: {
      maze.circle1 = 8;
      maze.circle2 = 34;
      maze.wallCount = 25;
      maze.walls = new Transition[maze.wallCount];
      maze.walls[0] = {3,4};
      maze.walls[1] = {7,8};
      maze.walls[2] = {8,9};
      maze.walls[3] = {9,10};
      maze.walls[4] = {13,14};
      maze.walls[5] = {15,16};
      maze.walls[6] = {16,17};
      maze.walls[7] = {19,20};
      maze.walls[8] = {20,21};
      maze.walls[9] = {21,22};
      maze.walls[24] ={24,25};
      maze.walls[10] = {25,26};
      maze.walls[11] = {26,27};
      maze.walls[12] = {28,29};
      maze.walls[13] = {30,31};
      maze.walls[14] = {32,33};
      maze.walls[15] = {6,12};
      maze.walls[16] = {1,7};
      maze.walls[17] = {13,19};
      maze.walls[18] = {2,8};
      maze.walls[19] = {14,20};
      maze.walls[20] = {27,33};
      maze.walls[21] = {4,10};
      maze.walls[22] = {22,28};
      maze.walls[23] = {11,17};
      break;
    }
    case 6: {
      maze.circle1 = 1;
      maze.circle2 = 31;
      maze.wallCount = 24;
      maze.walls = new Transition[maze.wallCount];
      maze.walls[0] = {12,18};
      maze.walls[1] = {1,7};
      maze.walls[2] = {13,17};
      maze.walls[3] = {25,31};
      maze.walls[4] = {2,8};
      maze.walls[5] = {20,26};
      maze.walls[6] = {26,32};
      maze.walls[7] = {3,9};
      maze.walls[8] = {9,15};
      maze.walls[9] = {15,21};
      maze.walls[10] ={21,27};
      maze.walls[11] = {17,23};
      maze.walls[12] = {6,7};
      maze.walls[13] = {7,8};
      maze.walls[14] = {10,11};
      maze.walls[15] = {13,14};
      maze.walls[16] = {16,17};
      maze.walls[17] = {19,20};
      maze.walls[18] = {21,22};
      maze.walls[19] = {24,25};
      maze.walls[20] = {26,27};
      maze.walls[21] = {28,29};
      maze.walls[22] = {33,34};
      maze.walls[23] = {22,28};
      break;
    }
    case 7: {
      maze.circle1 = 14;
      maze.circle2 = 33;
      maze.wallCount = 25;
      maze.walls = new Transition[maze.wallCount];
      maze.walls[0] = {6,7};
      maze.walls[1] = {7,8};
      maze.walls[2] = {12,13};
      maze.walls[3] = {13,14};
      maze.walls[4] = {18,19};
      maze.walls[5] = {22,23};
      maze.walls[6] = {26,27};
      maze.walls[7] = {28,29};
      maze.walls[8] = {30,31};
      maze.walls[9] = {33,34};
      maze.walls[10] ={7,13};
      maze.walls[11] = {19,25};
      maze.walls[12] = {2,8};
      maze.walls[13] = {14,20};
      maze.walls[14] = {20,26};
      maze.walls[15] = {26,32};
      maze.walls[16] = {3,9};
      maze.walls[17] = {9,15};
      maze.walls[18] = {15,21};
      maze.walls[19] = {21,27};
      maze.walls[20] = {4,10};
      maze.walls[21] = {10,16};
      maze.walls[22] = {22,28};
      maze.walls[23] = {5,11};
      maze.walls[24] = {11,17};
      break;
    }
    case 8: {
      maze.circle1 = 7;
      maze.circle2 = 26;
      maze.wallCount = 24;
      maze.walls = new Transition[maze.wallCount];
      maze.walls[0] = {1,2};
      maze.walls[1] = {3,4};
      maze.walls[2] = {6,7};
      maze.walls[3] = {7,8};
      maze.walls[4] = {8,9};
      maze.walls[5] = {10,11};
      maze.walls[6] = {12,13};
      maze.walls[7] = {13,14};
      maze.walls[8] = {15,16};
      maze.walls[9] = {20,21};
      maze.walls[10] ={22,23};
      maze.walls[11] = {24,25};
      maze.walls[12] = {25,26};
      maze.walls[13] = {27,28};
      maze.walls[14] = {28,29};
      maze.walls[15] = {30,31};
      maze.walls[16] = {13,19};
      maze.walls[17] = {14,20};
      maze.walls[18] = {26,32};
      maze.walls[19] = {9,15};
      maze.walls[20] = {21,27};
      maze.walls[21] = {27,33};
      maze.walls[22] = {10,16};
      maze.walls[23] = {16,22};
      maze.walls[24] = {5,11};
      break;
    }
    
  }
  
  return maze;
}
