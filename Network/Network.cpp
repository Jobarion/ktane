#include "Network.h"
#include "Arduino.h"
#define BROADCAST_ADDR -1

typedef void (*callback_func)(Network* network, const uint8_t src, const uint8_t *buffer, const size_t length);

Network::Network(unsigned long baud) {
  Serial.begin(baud);
}

void listenMaster(Network* network, const uint8_t src, const uint8_t *buffer, const size_t length) {
  
}
void listenSlave(Network* network, const uint8_t src, const uint8_t *buffer, const size_t length) {
  if(length == 1) {
    network->_id = buffer[0] + 1;
    uint8_t data[] = {buffer[0] + 1};
    network->send_raw(0, -1, data, 1);
  }
}

void Network::init(bool master) {
  if(master) {
    uint8_t data[] = {0};
    send_raw(0, -1, data, 1);
    receive(true, *listenMaster);
  }
  else {
    receive(true, *listenSlave);
  }
}


void Network::update(callback_func callback) {
  receive(false, callback);
}

void Network::receive(bool blocking, callback_func callback) {
  if(awaitingPacket && Serial.available() < 3) {
    if(!blocking) return;
    while(Serial.available() < 3);
  }
  if(awaitingPacket) {
    if(Serial.available() < 3) {
      if(!blocking) return;
      while(Serial.available() < 3);
    }
    dst = Serial.read();
    src = Serial.read();
    required = Serial.read();
    //Discard
    if(src == _id || required > MESSAGE_SIZE_MAX) {
      while(required > 0) {
        Serial.read();
        required--;
      }
      if(blocking) {
        receive(blocking, callback);
      }
      return;
    }
    awaitingPacket = false;
  }
  do {
    size_t actual = Serial.readBytes(_buffer + (sizeof(uint8_t) * offset), required);
    required -= actual;
    offset += actual;
  } while(required > 0 && blocking);
  if(required > 0) return;
  awaitingPacket = true;
  if(dst == -1 || dst != _id) {
    send_raw(dst, src, _buffer, offset);
  }
  if(dst == -1 || dst == _id) {
    callback(this, src, _buffer, offset);
  }
  else if(blocking) {
    receive(blocking, callback);
  }
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
