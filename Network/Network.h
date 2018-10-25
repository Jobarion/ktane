#include "Arduino.h"


class Network {
    public:
        Network(unsigned long baud);
        int8_t _id = 0;
        void init(bool master);
        size_t send(const int8_t dst, const uint8_t *buffer, const size_t length);
        size_t broadcast(const uint8_t *buffer, const size_t length);
        size_t send_raw(const int8_t dst, const uint8_t src, const uint8_t *buffer, const size_t length);
        void update(void (*callback)(Network* network, const uint8_t *buffer, const size_t length));
        void receive(bool blocking, void (*callback)(Network* network, const uint8_t *buffer, const size_t length));
        enum {MESSAGE_SIZE_MAX=128};
        
    private:
        bool awaitingPacket = true;
        uint16_t offset = 0;
        uint8_t required = 0;
        int8_t dst = 0;
        uint8_t src = 0;
        uint8_t _buffer[MESSAGE_SIZE_MAX];
        
};
