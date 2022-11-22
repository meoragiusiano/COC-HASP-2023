//TODO: implement motor limits, bring SD code to SDCard.cpp, bring variables to INO, improve detection algorithm

#include "Camera.h"
#include "Motors.h"
#include "SDCard.h"

float ImageDelay = 0;
bool MotorsActive = true;
bool TestOutput = true;
int TestOutputFiles = 0;

void setup() 
{
  Serial.begin(9600);

  InitSDCard();
  InitCamera();

  if (MotorsActive)
    InitMotors();
}

void loop() 
{
  //Taking an image
  int imageData[320][240];
  int (*imageDataPtr)[320][240] = &imageData;

  for (int i = 0; i < 240; i++)
    for (int j = 0; j < 320; j++)
      imageData[j][i] = -1;

  TakePicture(imageDataPtr, true);

  //Finding light patches
  int currCameraCenter[2] = {240 / 2, 320 / 2};
  int targetCameraCenter[2] = {-1, -1};

  FindLargestLightPatch(imageDataPtr, targetCameraCenter, true);

  //Running motors
  if (MotorsActive)
  {
    float travelRequiredX = targetCameraCenter[0] - currCameraCenter[0];
    float travelRequiredY = targetCameraCenter[1] - currCameraCenter[1];
    int motorASteps = abs((int)(travelRequiredX)) / 7;
    int motorBSteps = abs((int)(travelRequiredY)) / 7;
    //int motorASteps = 5;
    //int motorBSteps = 5;
    float motorADir = 0;
    float motorBDir = 0;

    if ((targetCameraCenter[0] == -1) || (targetCameraCenter[1] == -1))
    {
      motorASteps = 0;
      motorBSteps = 0;
    }

    if (travelRequiredX < 0)
      motorADir = 1;
    else if (travelRequiredX > 0)
      motorADir = 0;
    else
      motorASteps = 0;

    if (travelRequiredY < 0)
      motorBDir = 0;
    else if (travelRequiredY > 0)
      motorBDir = 1;
    else
      motorBSteps = 0;

    RunMotorA(motorADir, motorASteps);
    RunMotorB(motorBDir, motorBSteps);
  }

  //Testing
  if (TestOutput)
  {
    String TESTFileName = "PTCH_" + String(TestOutputFiles) + ".txt";
    char buff1[256];
    
    TESTFileName.toCharArray(buff1, TESTFileName.length() + 1);
    buff1[TESTFileName.length()] = '\0';

    if (SD.exists(buff1) == 1)
      SD.remove(buff1);

    File TEST = SD.open(buff1, FILE_WRITE);

    for (int i = 0; i < 240; i++)
    {
      for (int j = 0; j < 320; j++)
      {
        int chars = 1;

        if (imageData[j][i] != -1)
        {
          String currImageData = String(imageData[j][i]);
          char buff[256];
        
          currImageData.toCharArray(buff, currImageData.length() + 1);
          buff[currImageData.length()] = '\0';

          TEST.write(buff);

          chars = currImageData.length();
        }
        else
        {
          TEST.write("l");
        }

        String spaces = "";

        for (int k = 0; k < (4 - chars); k++)
          spaces += " ";

        char buff[256];
        
        spaces.toCharArray(buff, spaces.length() + 1);
        buff[spaces.length()] = '\0';

        TEST.write(buff);
      }

      TEST.write("\n");
    }

    TEST.write("\n\n");

    String posData = String(currCameraCenter[0]) + ", " + String(currCameraCenter[1]) + "\n" + String(targetCameraCenter[0]) + ", " + String(targetCameraCenter[1]);
    char buff[256];
      
    posData.toCharArray(buff, posData.length() + 1);
    buff[posData.length()] = '\0';

    TEST.write(buff);

    TEST.close();

    TestOutputFiles++;
  }

  delay(ImageDelay * 1000);
}