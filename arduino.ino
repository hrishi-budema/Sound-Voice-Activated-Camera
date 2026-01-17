#include <Servo.h>

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

unsigned long lastMove = 0;
const int stepDelayMs = 10;

void setup() {
  Serial.begin(19200);

  motor1.attach(motorPin1);
  motor2.attach(motorPin2);

  motor1.write(m1);
  motor2.write(m2);

  Serial.println("=== SERVO TEST (19200 baud) ===");
  Serial.print("M1 range: "); Serial.print(M1_LEFT); Serial.print(".."); Serial.println(M1_RIGHT);
  Serial.print("M2 range: "); Serial.print(M2_MIN);  Serial.print(".."); Serial.println(M2_MAX);
  Serial.println("Motor2 steps after each full M1 cycle (left->right->left).");
}

void stepMotor2() {
  int old = m2;

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

  Serial.print("[CYCLE] M2: ");
  Serial.print(old);
  Serial.print(" -> ");
  Serial.println(m2);
}

void loop() {
  unsigned long now = millis();

  if (now - lastMove >= (unsigned long)stepDelayMs) {
    m1 += m1Dir;

    if (m1 >= M1_RIGHT) {
      m1 = M1_RIGHT;
      m1Dir = -1;
      hitRightThisCycle = true;
      Serial.println("[M1] Hit RIGHT, returning.");
    }
    else if (m1 <= M1_LEFT) {
      m1 = M1_LEFT;
      m1Dir = +1;

      if (hitRightThisCycle) {
        hitRightThisCycle = false;
        stepMotor2();
      }
    }

    motor1.write(m1);
    lastMove = now;
  }
}
