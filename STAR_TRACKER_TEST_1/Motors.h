#ifndef _Motors_h_
#define _Motors_h_

#include "Arduino.h"

void RunMotorA(int direction, int steps);
void RunMotorB(int direction, int steps);
void InitMotors();

#endif