#include "Arduino.h"


class Network {
    public:
        Network(unsigned long baud);
        bool READ_OWN_BROADCASTS = false;
        bool READ_OWN_PACKETS = false;
        bool READ_FORWARDED_PACKETS = false;
        uint8_t _id = 0;
        uint8_t networkSize = 1;
        void init(bool master);
        size_t send(const int8_t dst, const uint8_t *buffer, const size_t length);
        size_t broadcast(const uint8_t *buffer, const size_t length);
        size_t send_raw(const int8_t dst, const uint8_t src, const uint8_t *buffer, const size_t length);
        bool receive(void (*callback)(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t *length));
        enum {MESSAGE_SIZE_MAX=128};
        
    private:
        bool readingPacket = false;
        bool discarding = false;
        uint16_t offset = 0;
        uint8_t required = 0;
        int8_t dst = 0;
        uint8_t src = 0;
        uint8_t _buffer[MESSAGE_SIZE_MAX];
        
};
