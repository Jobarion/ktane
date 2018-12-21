#include <Network.h>

Network* network;

enum phase {
  INIT,
  INIT_SYNC,
  RUNNING,
  OVER
};

struct settings {
  uint8_t maxStrikes;
  uint8_t snd_trn;
  uint8_t bob_rca;
  uint8_t battery_aa : 4;
  uint8_t battery_d : 4;
};

union settings_u {
  settings settings;
  uint8_t serialized[sizeof(struct settings)];
};

struct state {
  phase phase = INIT;
  uint8_t strikes = 0;
};

state gameState;
settings settings;


void setup() {
  network = new Network(9600);
  network->init(true);
  settings.maxStrikes = 3;
  settings.snd_trn = 0b00000000;
  settings.bob_rca = 0b00000000;
  settings.battery_aa = 1;
  settings.battery_d = 2;
  //INIT MODULES
  initStart();
  initSync();
}

void loop() {
  if(gameState.phase == OVER) return;
  network->update(*acceptPacket);
}

void initStart() {
  uint8_t buffer[sizeof(struct settings) + 1];
  buffer[0] = 0; //INIT
  union settings_u serializer;
  serializer.settings = settings;
  memcpy(&buffer[1], serializer.serialized, sizeof(struct settings));
  network->broadcast(buffer, sizeof(struct settings) + 1);
}

void initSync() {
  gameState.phase = INIT_SYNC;
  uint8_t buffer[1];
  buffer[0] = 1;
  network->send(network->networkSize - 1, buffer, 1);
}

void acceptPacket(Network* network, const uint8_t src, const uint8_t *buffer, const size_t length) {
  if(length == 0 || buffer == NULL) return;
  switch(buffer[0]) {
    case 0:
    case 2: terminate(); break;
    case 1: startGame(); break;
    case 3: gameState.phase = OVER; break;
    case 4: strike(); break;
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
  uint8_t buffer[] = {2};
  network->broadcast(buffer, 1);
}

void strike() {
  if(gameState.phase == RUNNING) {
    gameState.strikes++;
    if(gameState.strikes >= settings.maxStrikes) {
      terminate();
    }
  }
}
