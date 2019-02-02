#include "Network.h"
#include "Arduino.h"
#define BROADCAST_ADDR -1

typedef void (*callback_func)(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t *length);

Network::Network(unsigned long baud) {
  Serial.begin(baud);
}

void listenMaster(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t *length) {
  network->networkSize = buffer[0] + 1;
}

void listenSlave(Network* network, const uint8_t src, const uint8_t dst, uint8_t *buffer, size_t *length) {
  if(*length == 1) {
    network->_id = buffer[0] + 1;
    uint8_t data[] = {(uint8_t)(buffer[0] + 1)};
    network->send_raw(0, -1, data, 1);
  }
}

void Network::init(bool master) {
  if(master) {
    uint8_t data[] = {0};
    send_raw(0, -1, data, 1);
    bool received = false;
    do {
        received = receive(*listenMaster);
    } while(!received);
  }
  else {
    bool received = false;
    do {
        received = receive(*listenSlave);
    } while(!received);
  }
}

bool Network::receive(callback_func callback) {
  if(discarding) {
    while(required > 0 && Serial.available() > 0) {
      Serial.read();
      required--;
    }
    discarding = required > 0;
    return false;
  }
  else if(!readingPacket) {
    if(Serial.available() < 3) {
        return false;
    }
    dst = Serial.read();
    src = Serial.read();
    required = Serial.read();
    //Discard
    if(src == _id) {
        if(dst == BROADCAST_ADDR && !READ_OWN_BROADCASTS) {
            discarding = true;
        }
        else if(!READ_OWN_PACKETS) {
            discarding = true;
        }
    }
    if(discarding || required > MESSAGE_SIZE_MAX) {
      discarding = true;
      receive(callback);
      return false;
    }
    readingPacket = true;
  }
  size_t actual = Serial.readBytes(_buffer + (sizeof(uint8_t) * offset), required);
  required -= actual;
  offset += actual;
  if(required > 0) return false;
  readingPacket = false;
  uint8_t nid = _id; //Save network ID because listenSlave changes it which causes issues in the init sequence
  if(dst == BROADCAST_ADDR || dst == nid || READ_FORWARDED_PACKETS) {
    callback(this, src, dst, _buffer, &offset);
  }
  if(offset != 0 && src != nid && (dst == BROADCAST_ADDR || dst != nid)) {
    send_raw(dst, src, _buffer, offset);
  }
  offset = 0;
  return true;
}

size_t Network::send(const int8_t dst, const uint8_t *buffer, const size_t len) {
  send_raw(dst, _id, buffer, len);
}

size_t Network::send_raw(const int8_t dst, const uint8_t src, const uint8_t *buffer, const size_t len) {
  if(buffer == NULL) return 0;
  size_t sent = 0;
  sent += Serial.write(dst);
  sent += Serial.write(src);
  sent += Serial.write(len);
  return sent + Serial.write(buffer, len);
}

size_t Network::broadcast(const uint8_t *buffer, const size_t len) {
  send(BROADCAST_ADDR, buffer, len);
}
