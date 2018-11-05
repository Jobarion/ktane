#include <Network.h>

#define MIN_LENGTH 3
#define MAX_LENGTH 6
#define FLASH_DURATION 400
#define FLASH_DELAY 300
#define FLASH_PAUSE 5000

Network* network;
const uint8_t buttons[] = {5, 2, 3, 4};//R B G Y
const uint8_t leds[] = {11, 8, 9, 10};//R B G Y
uint8_t sequenceLength;
uint8_t *ledSequence;
bool serialVowel = false;
uint8_t lookupTable[3][4];
uint8_t strikes = 0;
uint8_t currentRound = 1;
uint8_t inputId = 0;
bool input = false;
bool done = false;

void setup() {
  network = new Network(9600);
  network->init(false);
  network->receive
  randomSeed(analogRead(0));
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  for(int i = 0; i < 4; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
    digitalWrite(leds[i], LOW);
    pinMode(leds[i], OUTPUT);
  }
  sequenceLength = random(MIN_LENGTH, MAX_LENGTH + 1);
  ledSequence = new uint8_t[sequenceLength];
  for(int i = 0; i < sequenceLength; i++) {
    ledSequence[i] = random(0, 4);
  }
  if(serialVowel) {
    lookupTable[0][0] = 1;
    lookupTable[0][1] = 0;
    lookupTable[0][2] = 3;
    lookupTable[0][3] = 2;

    lookupTable[1][0] = 3;
    lookupTable[1][1] = 2;
    lookupTable[1][2] = 1;
    lookupTable[1][3] = 0;

    lookupTable[2][0] = 2;
    lookupTable[2][1] = 0;
    lookupTable[2][2] = 3;
    lookupTable[2][3] = 1;
  }
  else {
    lookupTable[0][0] = 1;
    lookupTable[0][1] = 3;
    lookupTable[0][2] = 2;
    lookupTable[0][3] = 0;

    lookupTable[1][0] = 0;
    lookupTable[1][1] = 1;
    lookupTable[1][2] = 3;
    lookupTable[1][3] = 2;

    lookupTable[2][0] = 3;
    lookupTable[2][1] = 2;
    lookupTable[2][2] = 1;
    lookupTable[2][3] = 0;
  }
}

bool interruptibleDelay(long m) {
  long t = millis();
  while(t + m > millis()) {
    for(int i = 0; i < 4; i++) {
      if(digitalRead(buttons[i]) == LOW) {
        return true;
      }
    }
  }
  return false;
}

void loop() {
  network->update(*onNetwork);
  if(done) return;
  if(!input) {
    for(int i = 0; !input && i < currentRound; i++) {
      uint8_t pin = leds[ledSequence[i]];
      digitalWrite(pin, HIGH);
      if(interruptibleDelay(FLASH_DURATION)) {
        input = true;
        digitalWrite(pin, LOW);
        return;
      }
      digitalWrite(pin, LOW);
      if(i < sequenceLength - 1) {
        if(interruptibleDelay(FLASH_DELAY)) {
          input = true;
          return;
        }
      }
    }
    if(interruptibleDelay(FLASH_PAUSE)) {
      input = true;
      return;
    }
  }
  else {
    inputLoop();
  }
}

int readButton() {
  for(int i = 0; i < 4; i++) {
    if(digitalRead(buttons[i]) == LOW) {
      return i;
    }
  }
  return -1;
}

void waitForNoButton() {
  delay(50);
  while(readButton() >= 0);
}

void inputLoop() {
  int button = readButton();
  if(button == -1) {
    return;
  }
  digitalWrite(leds[button], HIGH);
  waitForNoButton();
  digitalWrite(leds[button], LOW);
  if(lookupTable[strikes][ledSequence[inputId]] == button) {
    inputId++;
    if(inputId == sequenceLength) {
      done = true;
      digitalWrite(leds[2], HIGH);
      return;
    }
    else if(inputId == currentRound) {
      inputId = 0;
      currentRound++;
      input = false;
      delay(1000);
    }
  }
  else {
    strikes++;
    input = false;
    digitalWrite(leds[0], HIGH);
    digitalWrite(leds[1], HIGH);
    digitalWrite(leds[2], HIGH);
    digitalWrite(leds[3], HIGH);
    delay(1000);
    digitalWrite(leds[0], LOW);
    digitalWrite(leds[1], LOW);
    digitalWrite(leds[2], LOW);
    digitalWrite(leds[3], LOW);
    delay(500);
    inputId = 0;
    if(strikes > 2) {
      done = true;
      digitalWrite(leds[0], HIGH);
      return;
    }
  }
}

void onNetwork(Network* network, const uint8_t *buffer, const size_t length) {
  if(length == NULL || length == 0) return;
  uint8_t packageType = buffer[0];
}
