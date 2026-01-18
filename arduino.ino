#include <Servo.h>

int stcp_Pin = 12;
int shcp_Pin = 11;
int ds_Pin   = 3;

const int LED_PIN       = 13;
const int espTriggerPin = 4;

const int soundDigitalPin = 2;
const int soundAnalogPin  = A0;

const int redLed     = 8;
const int triggerled = 7;

Servo motor1;
Servo motor2;

const int motorPin1 = 10;
const int motorPin2 = 9;

const int M1_LEFT  = 10;
const int M1_RIGHT = 170;

const int M2_MIN = 30;
const int M2_MAX = 120;

const int M2_STEP = 10;
const int M2_MODE_BOUNCE = 1;

int m1 = M1_LEFT;
int m1Dir = +1;
bool hitRightThisCycle = false;

int m2 = M2_MIN;
int m2Dir = +1;

unsigned long lastMoveTime = 0;
const int stepDelayMs = 10;

int lastState = LOW;
bool waitingSecond = false;
unsigned long firstClapTime = 0;
unsigned long lastEdgeTime = 0;

const unsigned long debounceMs     = 40;
const unsigned long doubleClapMs   = 200;
const unsigned long pauseMs        = 3000;
const unsigned long clapBlinkMs    = 50;

bool paused = false;
unsigned long pauseEndTime = 0;

const unsigned long espPulseMs = 300;
const unsigned long espGapMs   = 700;

bool espPulseActive = false;
bool espGapActive   = false;
unsigned long espPulseEnd = 0;
unsigned long espGapEnd   = 0;

unsigned long triggerLockoutEnd = 0;
const unsigned long triggerLockoutMs = 1500;

bool ledBlinkActive = false;
unsigned long ledBlinkEnd = 0;

unsigned long lastVU = 0;
const unsigned long vuUpdateMs = 30;

int baseline = 0;
int peak = 0;
unsigned long lastDecay = 0;
const unsigned long decayMs = 50;

void write595(uint8_t value) {
  digitalWrite(stcp_Pin, LOW);
  shiftOut(ds_Pin, shcp_Pin, LSBFIRST, value);
  digitalWrite(stcp_Pin, HIGH);
}

uint8_t levelToBarByte(int level) {
  if (level <= 0) return 0x00;
  if (level >= 8) return 0xFF;
  return (uint8_t)((1 << level) - 1);
}

void startClapBlink(unsigned long now) {
  digitalWrite(LED_PIN, HIGH);
  ledBlinkActive = true;
  ledBlinkEnd = now + clapBlinkMs;
}

void updateClapBlink(unsigned long now) {
  if (ledBlinkActive && now >= ledBlinkEnd) {
    digitalWrite(LED_PIN, LOW);
    ledBlinkActive = false;
  }
}

void startEspPulse(unsigned long now) {
  digitalWrite(espTriggerPin, HIGH);
  espPulseActive = true;
  espGapActive = false;
  espPulseEnd = now + espPulseMs;
}

void updateEspPulse(unsigned long now) {
  if (espPulseActive && now >= espPulseEnd) {
    digitalWrite(espTriggerPin, LOW);
    espPulseActive = false;
    espGapActive = true;
    espGapEnd = now + espGapMs;
  }
  if (espGapActive && now >= espGapEnd) {
    espGapActive = false;
  }
}

void stepMotor2() {
  if (M2_MODE_BOUNCE) {
    m2 += (m2Dir * M2_STEP);
    if (m2 >= M2_MAX) { m2 = M2_MAX; m2Dir = -1; }
    if (m2 <= M2_MIN) { m2 = M2_MIN; m2Dir = +1; }
  } else {
    m2 += M2_STEP;
    if (m2 > M2_MAX) m2 = M2_MAX;
    if (m2 < M2_MIN) m2 = M2_MIN;
  }
  motor2.write(m2);
}

void handleServo(unsigned long now) {
  if (paused) {
    if (now >= pauseEndTime) {
      paused = false;
      digitalWrite(redLed, LOW);
      digitalWrite(triggerled, LOW);
    }
    return;
  }

  if (now - lastMoveTime >= (unsigned long)stepDelayMs) {
    m1 += m1Dir;

    if (m1 >= M1_RIGHT) {
      m1 = M1_RIGHT;
      m1Dir = -1;
      hitRightThisCycle = true;
    } else if (m1 <= M1_LEFT) {
      m1 = M1_LEFT;
      m1Dir = +1;
      if (hitRightThisCycle) {
        hitRightThisCycle = false;
        stepMotor2();
      }
    }

    motor1.write(m1);
    lastMoveTime = now;
  }
}

void handleVUMeter(unsigned long now) {
  if (now - lastVU < vuUpdateMs) return;
  lastVU = now;

  int v = analogRead(soundAnalogPin);
  baseline = (baseline * 31 + v) / 32;

  int amp = v - baseline;
  if (amp < 0) amp = -amp;

  if (amp > peak) peak = amp;
  if (now - lastDecay >= decayMs) {
    lastDecay = now;
    if (peak > 0) peak -= 2;
    if (peak < 0) peak = 0;
  }

  int level = map(peak, 0, 200, 0, 8);
  if (level < 0) level = 0;
  if (level > 8) level = 8;

  write595(levelToBarByte(level));
}

void handleClap(unsigned long now) {
  int state = digitalRead(soundDigitalPin);

  if (now - lastEdgeTime < debounceMs) {
    lastState = state;
    return;
  }

  if (state == HIGH && lastState == LOW) {
    lastEdgeTime = now;
    startClapBlink(now);

    if (now < triggerLockoutEnd) {
      lastState = state;
      return;
    }

    if (waitingSecond && (now - firstClapTime <= doubleClapMs)) {
      paused = true;
      pauseEndTime = now + pauseMs;

      digitalWrite(redLed, HIGH);
      digitalWrite(triggerled, HIGH);

      startEspPulse(now);

      triggerLockoutEnd = now + triggerLockoutMs;
      waitingSecond = false;
    } else {
      waitingSecond = true;
      firstClapTime = now;
    }
  }

  if (waitingSecond && (now - firstClapTime > doubleClapMs)) {
    waitingSecond = false;
  }

  lastState = state;
}

void setup() {
  pinMode(soundDigitalPin, INPUT);
  pinMode(soundAnalogPin, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(triggerled, OUTPUT);

  pinMode(stcp_Pin, OUTPUT);
  pinMode(shcp_Pin, OUTPUT);
  pinMode(ds_Pin, OUTPUT);
  write595(0x00);

  pinMode(espTriggerPin, OUTPUT);
  digitalWrite(espTriggerPin, LOW);

  motor1.attach(motorPin1);
  motor2.attach(motorPin2);

  m1 = M1_LEFT;
  m1Dir = +1;
  hitRightThisCycle = false;

  m2 = M2_MIN;
  m2Dir = +1;

  motor1.write(m1);
  motor2.write(m2);

  lastMoveTime = millis();
  baseline = analogRead(soundAnalogPin);
}

void loop() {
  unsigned long now = millis();

  handleClap(now);
  handleServo(now);
  handleVUMeter(now);

  updateClapBlink(now);
  updateEspPulse(now);
}
