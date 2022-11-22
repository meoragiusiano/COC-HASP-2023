#ifndef _Camera_h_
#define _Camera_h_

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>

#include "memorysaver.h"
#include "SDCard.h"

struct LightPatch;

void FindLargestLightPatch(int (*imageDataPtr)[320][240], int largestPatchPos[], bool saveTargets);
void ReadBMPBurst(int (*imageDataPtr)[320][240], bool sendData);
//void ReadJPEGBurst(bool sendData);
void TakePicture(int (*imageDataPtr)[320][240], bool sendData);
void InitCamera();

#endif