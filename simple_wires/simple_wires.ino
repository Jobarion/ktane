#define MAX_CABLES 6
#define COMP_RES_OHM 1000
#define V_IN 5.0
#define WHITE_MAX 90
#define YELLOW_MAX 320
#define RED_MAX 990
#define BLUE_MAX 9990
#define BLACK_MAX 100000


enum color {
  WHITE,
  YELLOW,
  RED,
  BLUE,
  BLACK,
  EMPTY
};

const uint8_t error_led = 0;

const uint8_t cable_pins[MAX_CABLES] = {
  2, 3, 4, 5, 6, 7
};

const uint8_t color_power_pins[5] = {
  8, 9, 10, 11, 12
};

const uint8_t resistor_read_pin = A0;

color cables[MAX_CABLES] = {
  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY
};

uint8_t toCut = -1;
uint8_t cableCount = 0;
uint8_t strikesLeft = 1;
uint8_t state = 0; //0 = WAITING, 1 = ARMED, 2 = SOLVED, 3 = LOST

// the setup routine runs once when you press reset:
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(error_led, INPUT);
  for(int i = 0; i < 5; i++) {
    pinMode(color_power_pins[i], INPUT);
    digitalWrite(color_power_pins[i], HIGH);
  }
  for(int i = 0; i < MAX_CABLES; i++) {
    cables[i] = readCable(cable_pins[i]);
    if(cables[i] == EMPTY) {
      break;
    }
    cableCount++;
  }
  if(cableCount < 3) {
    return;
  }
  bool even = false;
  toCut = calculateToCut(cableCount, even);
  for(int i = 0; i < 5; i++) {
    pinMode(color_power_pins[i], OUTPUT);
    digitalWrite(color_power_pins[i], LOW);
  }
  state = 1;
}

color readCable(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  float vout = analogRead(resistor_read_pin) * (V_IN / 1023.0);
  pinMode(pin, INPUT);
  float r2 = COMP_RES_OHM * (1.0 / ((V_IN / vout) - 1.0));
  if(r2 < WHITE_MAX) {
    return WHITE;
  }
  if(r2 < YELLOW_MAX) {
    return YELLOW;
  }
  if(r2 < RED_MAX) {
    return RED;
  }
  if(r2 < BLUE_MAX) {
    return BLUE;
  }
  if(r2 < BLACK_MAX) {
    return BLACK;
  }
  return EMPTY;
}

uint8_t calculateToCut(int cableCount, bool sEven) {
  switch(cableCount) {
    case 3: {
      uint8_t rc = 0;
      uint8_t bc = 0;
      uint8_t lb = 0;
      for(int i = 0; i < 3; i++) {
        if(cables[i] == RED) {
          rc++;
        }
        else if(cables[i] == BLUE) {
          bc++;
          lb = i;
        }
      }
      if(rc == 0) {
        return 1;
      }
      if(cables[2] == WHITE) {
        return 2;
      }
      if(bc > 1) {
        return lb;
      }
      return 2;
    }
    case 4: {
      uint8_t rc = 0;
      uint8_t lr = 0;
      uint8_t bc = 0;
      uint8_t yc = 0;
      for(int i = 0; i < 4; i++) {
        if(cables[i] == RED) {
          rc++;
          lr = i;
        }
        else if(cables[i] == BLUE) {
          bc++;
        }
        else if(cables[i] == YELLOW) {
          yc++;
        }
      }
      if(rc > 1 && !sEven) {
        return lr;
      }
      if(cables[3] == YELLOW && rc == 0) {
        return 0;
      }
      if(bc == 1) {
        return 0;
      }
      if(yc > 1) {
        return 3;
      }
      return 1;
    }
    case 5: {
      uint8_t bc = 0;
      uint8_t rc = 0;
      uint8_t yc = 0;
      for(int i = 0; i < 5; i++) {
        if(cables[i] == RED) {
          rc++;
        }
        else if(cables[i] == BLACK) {
          bc++;
        }
        else if(cables[i] == YELLOW) {
          yc++;
        }
      }
      if(!sEven && cables[4] == BLACK) {
        return 3;
      }
      if(rc == 1 && yc > 1) {
        return 0;
      }
      if(bc == 0) {
        return 1;
      }
      return 0;
    }
    case 6: {
      uint8_t rc = 0;
      uint8_t yc = 0;
      uint8_t wc = 0;
      for(int i = 0; i < 5; i++) {
        if(cables[i] == RED) {
          rc++;
        }
        else if(cables[i] == YELLOW) {
          yc++;
        }
        else if(cables[i] == WHITE) {
          wc++;
        }
      }
      if(yc == 0 && !sEven) {
        return 2;
      }
      if(yc == 1 && wc > 1) {
        return 3;
      }
      if(rc == 0) {
        return 5;
      }
      return 3;
    }
    default: {
      return -1;
    }
  }
}

// the loop routine runs over and over again forever:
void loop() {
  if(state != 1) return;
  for(int i = 0; i < cableCount; i++) {
    if(cables[i] == EMPTY) continue;
    digitalWrite(cable_pins[i], HIGH);
    if(digitalRead(cable_pins[i]) == HIGH) {
      cables[i] = EMPTY;
      if(toCut == i) {
        digitalWrite(LED_BUILTIN, LOW);
        state = 2;
        break;
      }
      else {
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(error_led, LOW);
        pinMode(error_led, OUTPUT);
        digitalWrite(error_led, LOW);
        state = 3;
        break;
      }
    }
  }
}
