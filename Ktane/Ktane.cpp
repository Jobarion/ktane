#include "Arduino.h"
#include "Ktane.h"

typedef void (*gameLoop_func)();
typedef void (*initGame_func)();
typedef void (*startGame_func)();

union settings_u {
  Settings settings;
  uint8_t serialized[sizeof(struct Settings)];
};

Ktane* global;

Ktane::Ktane(Network* network, initGame_func initGame = NULL, startGame_func startGame = NULL, gameLoop_func gameLoop = NULL, void (*networkCallback)(Network* network, const uint8_t src, const uint8_t *buffer, const size_t length) = NULL) {
  Ktane::network = network;
  Ktane::initGame = initGame;
  Ktane::startGame = startGame;
  Ktane::gameLoop = gameLoop;
  Ktane::networkCallback = networkCallback;
  global = this;
}


void acceptPacketWrapper(Network* network, const uint8_t src, const uint8_t *buffer, const size_t length) {
  global->_acceptPacket(network, src, buffer, length);
}

void Ktane::refresh() {
  network->receive(*acceptPacketWrapper);
  if(gameState.phase == RUNNING) {
    if(Ktane::gameLoop != NULL)
      Ktane::gameLoop();
  }
}

void Ktane::_acceptPacket(Network* network, const uint8_t src, const uint8_t *buffer, const size_t length) {
  if(length == 0 || buffer == NULL) return;
  switch(buffer[0]) {
    case 0: {
      gameState.phase = INIT;
      union settings_u serializer;
      //settings_u.serialized = &(buffer[1]);
      Ktane::settings = serializer.settings;
      if(Ktane::initGame != NULL)
        Ktane::initGame();
      gameState.phase = INIT_DONE;
      break;
    }
    case 1: {
      gameState.phase = INIT_SYNC;
      uint8_t buffer[1];
      buffer[0] = 1;
      network->send(network->networkSize - 1, buffer, 1);
      break;
    }
    case 2: {
      gameState.phase = RUNNING;
      if(Ktane::startGame != NULL)
        Ktane::startGame();
      break;
    }
    case 3: {
      gameState.phase = OVER;
      break;
    }
  }
  if(Ktane::networkCallback != NULL) {
    Ktane::networkCallback(network, src, buffer, length);
  }
}