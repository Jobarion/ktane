#include "Arduino.h"
#include "Ktane.h"

typedef void (*gameLoop_func)();
typedef void (*initGame_func)();
typedef void (*startGame_func)();
typedef void (*cleanupGame_func)();

union settings_u {
  Settings settings;
  uint8_t serialized[sizeof(struct Settings)];
};

Ktane* global;

Ktane::Ktane(Network* network, initGame_func initGame = NULL, startGame_func startGame = NULL, gameLoop_func gameLoop = NULL, cleanupGame_func cleanupGame = NULL, void (*networkCallback)(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t* length) = NULL) {
  Ktane::network = network;
  Ktane::initGame = initGame;
  Ktane::startGame = startGame;
  Ktane::gameLoop = gameLoop;
  Ktane::cleanupGame = cleanupGame;
  Ktane::networkCallback = networkCallback;
  global = this;
}


void acceptPacketWrapper(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t* length) {
  global->_acceptPacket(network, src, dst, buffer, length);
}

void Ktane::refresh() {
  network->receive(*acceptPacketWrapper);
  if(gameState.phase == RUNNING) {
    if(Ktane::gameLoop != NULL)
      Ktane::gameLoop();
  }
}

void Ktane::_acceptPacket(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t* length) {
  if(*length == 0 || buffer == NULL) return;
  if(src != network->_id) {
    switch(buffer[0]) {
      case 0: { //INIT
        gameState.phase = INIT;
        union settings_u serializer;
        //settings_u.serialized = &(buffer[1]);
        Ktane::settings = serializer.settings;
        if(Ktane::initGame != NULL)
          Ktane::initGame();
        gameState.phase = INIT_DONE;
        break;
      }
      case 1: { //INIT_SYNC
        if(*length != 2) break;
        gameState.phase = INIT_SYNC;
        uint8_t out[2];
        out[0] = 1;
        out[1] = buffer[1] + (solvable ? 1 : 0);
        network->send(network->_id - 1, out, 2);
        break;
      }
      case 2: { //START
        gameState.phase = RUNNING;
        if(Ktane::startGame != NULL)
          Ktane::startGame();
        break;
      }
      case 3: { //OVER
        gameState.phase = OVER;
        if(Ktane::cleanupGame != NULL) {
          cleanupGame();
        }
        break;
      }
      case 5: { //MODULE_DISCOVERY
        if(*length < 3) break;
        uint16_t mid = (buffer[1] << 8) | buffer[2];
        if(mid == KTANE_MODULE_ID) {
          buffer[*length] = network->_id;
          (*length)++;
        }
        break;
      }
    }
  }
  if(Ktane::networkCallback != NULL) {
    Ktane::networkCallback(network, src, dst, buffer, length);
  }
}