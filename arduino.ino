#include <Servo.h>

int stcp_Pin = 12;
int shcp_Pin = 11;
int ds_Pin   = 3;

const int LED_PIN       = 13;
const int espTriggerPin = 4;

Servo motor1;
Servo motor2;

const int motorPin  = 10;
const int motorPin2 = 9;

const int RIGHT  = 180;
const int LEFT   = 0;

int currentAngle = 90;
int direction = 1;
unsigned long lastMoveTime = 0;
const int stepDelayMs = 10;

int motor2Angle = 90;
const int motor2Step = 10;
bool reachedRightThisCycle = false;

bool paused = false;
unsigned long pauseEndTime = 0;

const int soundPin   = 2;
const int redLed     = 8;
const int triggerled = 7;

int lastState = LOW;
bool waitingSecond = false;
unsigned long firstClapTime = 0;
unsigned long lastEdgeTime = 0;

const unsigned long debounceMs     = 40;
const unsigned long doubleClapMs   = 300;
const unsigned long pauseMs        = 3000;
const unsigned long clapBlinkMs    = 50;

const unsigned long espPulseMs     = 300;
const unsigned long espGapMs       = 700;
bool espPulseActive = false;
bool espGapActive   = false;
unsigned long espPulseEnd = 0;
unsigned long espGapEnd   = 0;

bool ledBlinkActive = false;
unsigned long ledBlinkEnd = 0;

unsigned long triggerLockoutEnd = 0;
const unsigned long triggerLockoutMs = 1500;

int shiftNumber = 0;
unsigned long lastShiftTime = 0;
const int shiftDelay = 90;

void setup() {
  pinMode(soundPin, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(triggerled, OUTPUT);

  pinMode(stcp_Pin, OUTPUT);
  pinMode(shcp_Pin, OUTPUT);
  pinMode(ds_Pin, OUTPUT);

  pinMode(espTriggerPin, OUTPUT);
  digitalWrite(espTriggerPin, LOW);

  motor1.attach(motorPin);
  motor2.attach(motorPin2);

  motor1.write(currentAngle);
  motor2.write(motor2Angle);

  lastMoveTime = millis();
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
    currentAngle += direction;

    if (currentAngle >= RIGHT) {
      currentAngle = RIGHT;
      direction = -1;
      reachedRightThisCycle = true;
    } else if (currentAngle <= LEFT) {
      currentAngle = LEFT;
      direction = 1;

      if (reachedRightThisCycle) {
        reachedRightThisCycle = false;
        motor2Angle += motor2Step;
        if (motor2Angle > RIGHT) motor2Angle = LEFT;
      }
    }

    motor1.write(currentAngle);
    motor2.write(motor2Angle);

    lastMoveTime = now;
  }
}

void handleClap(unsigned long now) {
  int state = digitalRead(soundPin);

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

void handleShiftRegister(unsigned long now) {
  if (now - lastShiftTime >= (unsigned long)shiftDelay) {
    digitalWrite(stcp_Pin, LOW);
    shiftOut(ds_Pin, shcp_Pin, LSBFIRST, shiftNumber);
    digitalWrite(stcp_Pin, HIGH);

    shiftNumber++;
    if (shiftNumber > 255) shiftNumber = 0;

    lastShiftTime = now;
  }
}

void loop() {
  unsigned long now = millis();

  handleClap(now);
  handleServo(now);
  handleShiftRegister(now);
  updateClapBlink(now);
  updateEspPulse(now);
}
