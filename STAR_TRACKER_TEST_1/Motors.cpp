#include "Motors.h"

//// MOTOR DIRECTIONS ////
// Motor A:
//  0 - CCW
//  1 - CW
// Motor B:
//  0 - CW
//  1 - CCW

const int PIN_MOTOR_A_STEP = 3;
const int PIN_MOTOR_A_DIR = 4;
const int PIN_MOTOR_B_STEP = 5;
const int PIN_MOTOR_B_DIR = 6;

const int SpinDelay = 12000;

void RunMotorA(int direction, int steps)
{
  digitalWrite(PIN_MOTOR_A_DIR, direction);

  for (int i = 0; i < steps; i++) {
    digitalWrite(PIN_MOTOR_A_STEP, HIGH);
    delayMicroseconds(SpinDelay);
    digitalWrite(PIN_MOTOR_A_STEP, LOW);
    delayMicroseconds(SpinDelay);
  }
}

void RunMotorB(int direction, int steps)
{
  digitalWrite(PIN_MOTOR_B_DIR, direction);

  for (int i = 0; i < steps; i++) {
    digitalWrite(PIN_MOTOR_B_STEP, HIGH);
    delayMicroseconds(SpinDelay);
    digitalWrite(PIN_MOTOR_B_STEP, LOW);
    delayMicroseconds(SpinDelay);
  }
}

void InitMotors()
{
  pinMode(PIN_MOTOR_A_STEP, OUTPUT);
  pinMode(PIN_MOTOR_A_DIR, OUTPUT);
  pinMode(PIN_MOTOR_B_STEP, OUTPUT);
  pinMode(PIN_MOTOR_B_DIR, OUTPUT);
}