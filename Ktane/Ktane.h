#include "Network.h"

enum Phase {
  WAITING,
  INIT,
  INIT_DONE,
  INIT_SYNC,
  RUNNING,
  OVER
};

struct State {
  Phase phase = INIT;
  uint8_t strikes = 0;
};

struct Settings {
  uint8_t maxStrikes;
  uint8_t snd_trn;
  uint8_t bob_rca;
  uint8_t battery_aa : 4;
  uint8_t battery_d : 4;
};

class Ktane {
    public:
        Ktane(Network* network, void (*initGame)(), void (*startGame)(), void (*gameLoop)(), void (*cleanupGame)(), void (*networkCallback)(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t* length));
        void refresh();
        State gameState;
        Settings settings;
        Network* network;
        bool solvable = true;
        uint16_t KTANE_MODULE_ID = -1;
        void _acceptPacket(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t* length);
    private:
        bool solved;
        void (*initGame)();
        void (*startGame)();
        void (*gameLoop)();
        void (*cleanupGame)();
        void (*networkCallback)(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t* length);
};