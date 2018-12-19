#include <Network.h>

Network* network;

enum phase {
  INIT,
  INIT_SYNC,
  RUNNING,
  OVER
};

struct state {
  uint8_t strikes = 0;
  uint8_t maxStrikes = 3;
  phase phase = INIT;
};

state gameState;

void setup() {
  network = new Network(9600);
  network->init(true);
  uint8_t buffer[10];
  buffer[0] = 0;
  network->broadcast(buffer, 1);
  buffer[0] = 1;
  network->send(1, buffer, 1);
}

void loop() {
  network->update(*acceptPacket);
}

void acceptPacket(Network* network, const uint8_t *buffer, const size_t length) {
  if(length == 0 || buffer == NULL) return;
  switch(buffer[0]) {
    case 0:
    case 2: terminate(); break;
    case 1: startGame(); break;
    case 3: gameState.phase = OVER; break;
  }
}

void terminate() {
  gameState.phase = OVER;
  uint8_t buffer[] = {3};
  network->broadcast(buffer, 1);
}

void startGame() {
  if(gameState.phase != INIT_SYNC) {
    terminate();
    return;
  }
  gameState.phase = RUNNING;
  uint8_t buffer[] = {3};
  network->broadcast(buffer, 1);
}
