#define SECONDS 720
#define D_1 14
#define SEG_A 2
#define TIMER_SEC 1518
#define TIMER_MSEC 64911

long hundSecs = 7200;

void setup() {
  for(int i = SEG_A; i < SEG_A + 7; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  noInterrupts();           // Alle Interrupts temporär abschalten
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = TIMER_SEC;             // Timer nach obiger Rechnung vorbelegen
  TCCR1B |= (1 << CS12);    // 256 als Prescale-Wert spezifizieren
  TIMSK1 |= (1 << TOIE1);   // Timer Overflow Interrupt aktivieren
  interrupts();
  pinMode(D_1 + 3, OUTPUT);
  digitalWrite(D_1 + 3, LOW);
}

int currentPin = D_1;

void loop() {
  int next = getNextDigit();
  int digit = calculateDigit(next);
  pinMode(currentPin, INPUT);
  clearPins();
  writePins(digit);
  pinMode(next, OUTPUT);
  digitalWrite(next, LOW);
  currentPin = next;
}

int getNextDigit() {
  return currentPin == D_1 + 3 ? D_1 : currentPin + 1;
}

void writePins(int digit) {
  switch(digit) {
    case 0: {
      PORTD = PORTD | B11111100;
      PORTB = PORTB & B11111110;
      break;
    }
    case 1: {
      PORTD = PORTD | B00011000;
      PORTB = PORTB & B11111110;
      break;
    }
    case 2: {
      PORTD = PORTD | B01101100;
      PORTB = PORTB | B00000001;
      break;
    }
    case 3: {
      PORTD = PORTD | B00111100;
      PORTB = PORTB | B00000001;
      break;
    }
    case 4: {
      PORTD = PORTD | B10011000;
      PORTB = PORTB | B00000001;
      break;
    }
    case 5: {
      PORTD = PORTD | B10110100;
      PORTB = PORTB | B00000001;
      break;
    }
    case 6: {
      PORTD = PORTD | B11110100;
      PORTB = PORTB | B00000001;
      break;
    }
    case 7: {
      PORTD = PORTD | B00011100;
      PORTB = PORTB & B11111110;
      break;
    }
    case 8: {
      PORTD = PORTD | B11111100;
      PORTB = PORTB | B00000001;
      break;
    }
    case 9: {
      PORTD = PORTD | B10111100;
      PORTB = PORTB | B00000001;
      break;
    }
  }
  /*
  digitalWrite(SEG_A, digit != 1 && digit != 4);
  digitalWrite(SEG_A + 1, digit <= 4 || digit >= 7);
  digitalWrite(SEG_A + 2, digit != 2);
  digitalWrite(SEG_A + 3, digit != 1 && digit != 4 && digit != 7);
  digitalWrite(SEG_A + 4, digit != 4 && digit % 2 == 0);
  digitalWrite(SEG_A + 5, digit == 0 || (digit >= 4 && digit <= 9 && digit != 7));
  digitalWrite(SEG_A + 6, digit >= 2 && digit <= 9 && digit != 7);
  */
}

void clearPins() {
  PORTD = PORTD & B00000011;
  PORTB = PORTB & B11111110;
  /*
  for(int i = 0; i < 7; i++) {
    digitalWrite(SEG_A + i, LOW);
  }
  */
}

int calculateDigit(int currentPin) {
  if(hundSecs >= 6000) {
    int secs = hundSecs / 100;
    switch(currentPin) {
      case D_1: {
        return secs / 600;
      }
      case D_1 + 1: {
        return (secs / 60) % 10;
      }
      case D_1 + 2: {
        return (secs % 60) / 10;
      }
      case D_1 + 3: {
        return secs % 10;
      }
    }
  }
  else {
    switch(currentPin) {
      case D_1: {
        return (hundSecs / 1000) % 10;
      }
      case D_1 + 1: {
        return (hundSecs / 100) % 10;
      }
      case D_1 + 2: {
        return (hundSecs / 10) % 10;
      }
      case D_1 + 3: {
        return hundSecs % 10;
      }
    }
  }
  return -1;
}

ISR(TIMER1_OVF_vect)        
{
  if(hundSecs > 6100) {
    hundSecs -= 100;
    TCNT1 = TIMER_SEC;
  }
  else if(hundSecs > 0){
    hundSecs--;
    TCNT1 = TIMER_MSEC;
  }
}
