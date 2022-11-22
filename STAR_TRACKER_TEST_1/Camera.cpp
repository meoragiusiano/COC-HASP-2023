#include "Camera.h"

const int PIN_CAMERA_CS = 7;
const int PIN_SD_CS = 8;

const uint8_t LightThreshold = 0xFF;
const int MaxLightPatches = 5000;
//bool SavingJPEG = false;

ArduCAM Camera(OV2640, PIN_CAMERA_CS);
int ImagesCreated = 0;
bool IsHeader = false;

#define BMPImageOffset 66
const uint8_t BMPHeader[BMPImageOffset] PROGMEM =
{
  0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00,
  0x00, 0x00
};

struct LightPatch
{
  float lX;
  float hX;
  float lY;
  float hY;
  //float sizeX;
  //float sizeY;
  //float centerX;
  //float centerY;

  //int prevDetectionsX[320];
  //int prevDetectionsXIdx;
};

void FindLargestLightPatch(int (*imageDataPtr)[320][240], int largestPatchPos[], bool saveTargets)
{
  Serial.println(F("ACK CMD Finding largest light patch... END"));

  LightPatch lightPatchBuff[MaxLightPatches];
  int lightPatchNum = 0;

  //Initializing all potential patches
  for (int i = 0; i < MaxLightPatches; i++)
  {
    lightPatchBuff[i].lX = -1;
    lightPatchBuff[i].lY = -1;
    lightPatchBuff[i].hX = -1;
    lightPatchBuff[i].hY = -1;
    //lightPatchBuff[i].sizeX = 0;
    //lightPatchBuff[i].sizeY = 0;
    //lightPatchBuff[i].centerX = -1;
    //lightPatchBuff[i].centerY = -1;

    //for (int j = 0; j < 320; j++)
    //  lightPatchBuff[i].prevDetectionsX[j] = 0;

    //lightPatchBuff[i].prevDetectionsXIdx = 0;
  }

  int prevLightPatchesX[320];
  //int newImageData[320][240];
  bool waitingForUp = false;
  int waitingLX = -1;

  //Running up-first algorithm
  for (int i = 0; i < 240; i++)
  {
    int currLightPatchesX[320];

    for (int j = 0; j < 320; j++)
    {
      int currLightValue = (*imageDataPtr)[j][i];
      int connectedPatchIdx = -1;
      bool connectedUp = false;
      bool connectedLeft = false;

      if (i > 0)
      {
        if (currLightValue == 1)
        {
          if (prevLightPatchesX[j] != -1)
          {
            connectedPatchIdx = prevLightPatchesX[j];
            connectedUp = true;
            waitingForUp = false;
          }
          else if ((j > 0) && (currLightPatchesX[j - 1] != -1))
          {
            connectedPatchIdx = currLightPatchesX[j - 1];
            connectedLeft = true;
            waitingForUp = false;
            waitingLX = -1;
          }
          else
          {
            if (j == 319)
              waitingForUp = false;
            else
              waitingForUp = true;
          }

          if (connectedUp)
          {
            if (i > lightPatchBuff[connectedPatchIdx].hY)
              lightPatchBuff[connectedPatchIdx].hY = i;
            if ((waitingLX != -1) && (waitingLX < lightPatchBuff[connectedPatchIdx].lX))
              lightPatchBuff[connectedPatchIdx].lX = waitingLX;

            waitingLX = -1;
          }
          else if (connectedLeft)
          {
            if (j < lightPatchBuff[connectedPatchIdx].lX)
              lightPatchBuff[connectedPatchIdx].lX = j;
            if (j > lightPatchBuff[connectedPatchIdx].hX)
              lightPatchBuff[connectedPatchIdx].hX = j;
          }
          else
          { 
            if (waitingForUp)
            {
              waitingLX = j;
            }
            else
            {
              if (lightPatchNum == MaxLightPatches)
              {
                Serial.println("ERROR: Light Patch Buffer exceeded maximum size!");
              }
              else
              {
                if (waitingLX != -1)
                  lightPatchBuff[lightPatchNum].lX = waitingLX;
                else
                  lightPatchBuff[lightPatchNum].lX = j;

                lightPatchBuff[lightPatchNum].hX = j;
                lightPatchBuff[lightPatchNum].lY = i;
                lightPatchBuff[lightPatchNum].hY = i;

                connectedPatchIdx = lightPatchNum;

                lightPatchNum++;
              }
            }
          }
        }
        else
        {
          if (waitingForUp)
          {
            lightPatchBuff[lightPatchNum].lX = waitingLX;
            lightPatchBuff[lightPatchNum].hX = j;
            lightPatchBuff[lightPatchNum].lY = i;
            lightPatchBuff[lightPatchNum].hY = i;

            connectedPatchIdx = lightPatchNum;

            lightPatchNum++;

            waitingForUp = false;
            waitingLX = -1;
          }
        }
      }
      else
      {
        if (currLightValue == 1)
        {
          if ((j > 0) && (currLightPatchesX[j - 1] != -1))
          {
            connectedPatchIdx = currLightPatchesX[j - 1];
            connectedLeft = true;
          }

          if (connectedLeft)
          {
            if (j < lightPatchBuff[connectedPatchIdx].lX)
              lightPatchBuff[connectedPatchIdx].lX = j;
            if (j > lightPatchBuff[connectedPatchIdx].hX)
              lightPatchBuff[connectedPatchIdx].hX = j;
          }
          else
          {
            if (lightPatchNum == MaxLightPatches)
            {
              Serial.println("ERROR: Light Patch Buffer exceeded maximum size!");
            }
            else
            {
              lightPatchBuff[lightPatchNum].lX = j;
              lightPatchBuff[lightPatchNum].hX = j;
              lightPatchBuff[lightPatchNum].lY = i;
              lightPatchBuff[lightPatchNum].hY = i;

              connectedPatchIdx = lightPatchNum;

              lightPatchNum++;
            }
          }
        }
      }

      currLightPatchesX[j] = connectedPatchIdx;
      (*imageDataPtr)[j][i] = connectedPatchIdx;
    }

    for (int j = 0; j < 320; j++)
      prevLightPatchesX[j] = currLightPatchesX[j];
  }

  //Finding largest patch
  if (lightPatchNum > 0)
  {
    float largestSize = (2 * (lightPatchBuff[0].hX - lightPatchBuff[0].lX)) + (2 * (lightPatchBuff[0].hY - lightPatchBuff[0].lY));
    float largestPosX = lightPatchBuff[0].lX + ((lightPatchBuff[0].hX - lightPatchBuff[0].lX) / 2);
    float largestPosY = lightPatchBuff[0].lY + ((lightPatchBuff[0].hY - lightPatchBuff[0].lY) / 2);

    for (int i = 1; i < lightPatchNum; i++)
    {
      float currSize = (2 * (lightPatchBuff[i].hX - lightPatchBuff[i].lX)) + (2 * (lightPatchBuff[i].hY - lightPatchBuff[i].lY));
      float currPosX = lightPatchBuff[i].lX + ((lightPatchBuff[i].hX - lightPatchBuff[i].lX) / 2);
      float currPosY = lightPatchBuff[i].lY + ((lightPatchBuff[i].hY - lightPatchBuff[i].lY) / 2);

      if (currSize > largestSize)
      {
        largestSize = currSize;
        largestPosX = currPosX;
        largestPosY = currPosY;
      }
    }

    largestPatchPos[0] = largestPosX;
    largestPatchPos[1] = largestPosY;
  }

  //Printing target diagrams
  if ((saveTargets) && (lightPatchNum > 0))
  {
    if (PIN_SD_CS >= 0)
    {
      String targetFileName = "TRG_" + String(ImagesCreated - 1) + ".bmp";
      char buff[100];
      
      targetFileName.toCharArray(buff, targetFileName.length() + 1);
      buff[targetFileName.length()] = '\0';

      if (SD.exists(buff) == 1)
        SD.remove(buff);

      File targetFile = SD.open(buff, FILE_WRITE);

      if (!targetFile)
      {
        Serial.println(F("ACK CMD ERROR: Unable to create target diagram on SD! END"));
      }
      else
      {
        for (int i = 0; i < BMPImageOffset; i++)
        {
          uint8_t send = pgm_read_byte(&BMPHeader[i]);

          targetFile.write(&send, 1);
        }

        for (int i = 239; i >= 0; i--)
        {
          for (int j = 0; j < 320; j++)
          {
            uint8_t send1 = 0x00;
            uint8_t send2 = 0x00;

            if ((j == largestPatchPos[0]) || (i == largestPatchPos[1]))
            {
              send1 = 0xFF;
            }
            else if ((j == (320 / 2)) || (i == (240 / 2)))
            {
              send2 = 0xFF;
            }

            targetFile.write(&send1, 1);
            targetFile.write(&send2, 1);
          }
        }

        uint8_t send1 = 0xBB;
        uint8_t send2 = 0xCC;

        targetFile.write(&send1, 1);
        targetFile.write(&send2, 1);

        targetFile.close();
      }
    }
  }

  Serial.println(F("ACK CMD Finished finding largest light patch. END"));
}

void ReadBMPBurst(int (*imageDataPtr)[320][240], bool sendData)
{
  Serial.println(F("ACK CMD Reading BMP burst... END"));

  uint32_t length = Camera.read_fifo_length();

  if (length >= MAX_FIFO_SIZE)
  {
    Serial.println(F("ACK CMD ERROR: Over size! END"));

    //Camera.clear_fifo_flag();

    return;
  }
  if (length == 0)
  {
    Serial.println(F("ACK CMD ERROR: Size is 0! END"));

    //Camera.clear_fifo_flag();

    return;
  }

  Camera.CS_LOW();
  Camera.set_fifo_burst();

  File outputFile;
  //File originalFile;

  if (sendData)
  {
    if (PIN_SD_CS >= 0)
    {
      String outputFileName = "OUT_" + String(ImagesCreated++) + ".bmp";
      //String originalFileName = "IMG_" + String(ImagesCreated - 1) + ".bmp";
      char buff1[100];
      //char buff2[100];
      
      outputFileName.toCharArray(buff1, outputFileName.length() + 1);
      buff1[outputFileName.length()] = '\0';
      //originalFileName.toCharArray(buff2, originalFileName.length() + 1);
      //buff2[originalFileName.length()] = '\0';

      Camera.CS_HIGH();

      if (SD.exists(buff1) == 1)
        SD.remove(buff1);
      //if (SD.exists(buff2) == 1)
      //  SD.remove(buff2);

      outputFile = SD.open(buff1, FILE_WRITE);
      //originalFile = SD.open(buff2, FILE_WRITE);

      Camera.CS_LOW();
      Camera.set_fifo_burst();

      if (!outputFile)
      {
        Serial.println(F("ACK CMD ERROR: Unable to create output BMP on SD! END"));

        //if (originalFile)
        //  originalFile.close();

        return;
      }
      /*if (!originalFile)
      {
        Serial.println(F("ACK CMD ERROR: Unable to create original BMP on SD! END"));

        outputFile.close();

        return;
      }*/
    }
  }

  /*if (sendData)
  {
    if (PIN_SD_CS >= 0)
    {
      uint8_t send1 = 0xFF;
      uint8_t send2 = 0xAA;

      Camera.CS_HIGH();

      outputFile.write(&send1, 1);
      outputFile.write(&send2, 1);
      //originalFile.write(&send1, 1);
      //originalFile.write(&send2, 1);

      Camera.CS_LOW();
      Camera.set_fifo_burst();
    }
    else
    {
      Serial.write(0xFF);
      Serial.write(0xAA);
    }
  }*/

  for (int i = 0; i < BMPImageOffset; i++)
  {
    if (sendData)
    {
      if (PIN_SD_CS >= 0)
      {
        uint8_t send = pgm_read_byte(&BMPHeader[i]);

        Camera.CS_HIGH();

        outputFile.write(&send, 1);
        //originalFile.write(&send, 1);

        Camera.CS_LOW();
        Camera.set_fifo_burst();
      }
      else
      {
        Serial.write(pgm_read_byte(&BMPHeader[i]));
      }
    }
  }

  //SPI.transfer(0x00);

  for (int i = 0; i < 240; i++)
  {
    for (int j = 0; j < 320; j++)
    {
      uint8_t VH = SPI.transfer(0x00);;
      uint8_t VL = SPI.transfer(0x00);;
      
      uint8_t send1 = 0x00;
      uint8_t send2 = 0x00;
      int insert = 0;

      if ((VH >= LightThreshold) && (VL >= LightThreshold))
      {
        send1 = VH;
        send2 = VL;
        insert = 1;
      }

      (*imageDataPtr)[j][239 - i] = insert;

      if (sendData)
      {
        if (PIN_SD_CS >= 0)
        {
          Camera.CS_HIGH();

          outputFile.write(&send1, 1);
          //delayMicroseconds(12);
          outputFile.write(&send2, 1);
          //delayMicroseconds(12);
          //originalFile.write(&send1, 1);
          //originalFile.write(&send2, 1);

          Camera.CS_LOW();
          Camera.set_fifo_burst();
        }
        else
        {
          Serial.write(VL);
          //delayMicroseconds(12);
          Serial.write(VH);
          //delayMicroseconds(12);
        }
      }
    }
  }

  if (sendData)
  {
    if (PIN_SD_CS >= 0)
    {
      uint8_t send1 = 0xBB;
      uint8_t send2 = 0xCC;

      Camera.CS_HIGH();

      outputFile.write(&send1, 1);
      outputFile.write(&send2, 1);
      //originalFile.write(&send1, 1);
      //originalFile.write(&send2, 1);

      Camera.CS_LOW();
      Camera.set_fifo_burst();
    }
    else
    {
      Serial.write(0xBB);
      Serial.write(0xCC);
    }
  }

  Camera.CS_HIGH();
  Camera.clear_fifo_flag();

  if (sendData)
  {
    if (PIN_SD_CS >= 0)
    {
      outputFile.close();
      //originalFile.close();
    }
  }

  Serial.println(F("ACK CMD BMP burst read! END"));

  return;
}

/*void ReadJPEGBurst(bool sendData)
{
  Serial.println(F("ACK CMD Reading JPEG burst... END"));

  uint32_t length = Camera.read_fifo_length();

  if (length >= MAX_FIFO_SIZE)
  {
    Serial.println(F("ACK CMD ERROR: Over size! END"));

    //Camera.clear_fifo_flag();

    return;
  }
  if (length == 0)
  {
    Serial.println(F("ACK CMD ERROR: Size is 0! END"));

    //Camera.clear_fifo_flag();

    return;
  }

  Camera.CS_LOW();
  Camera.set_fifo_burst();

  uint8_t temp = SPI.transfer(0x00);
  uint8_t temp_last = 0;
  
  File outputFile;

  if (sendData)
  {
    if (PIN_SD_CS >= 0)
    {
      String outputFileName = "IMG_" + String(ImagesCreated++) + ".jpg";
      char buff[100];
      
      outputFileName.toCharArray(buff, outputFileName.length() + 1);
      buff[outputFileName.length()] = '\0';

      Camera.CS_HIGH();

      if (SD.exists(buff) == 1)
        SD.remove(buff);

      outputFile = SD.open(buff, FILE_WRITE);

      Camera.CS_LOW();
      Camera.set_fifo_burst();

      if (!outputFile)
      {
        Serial.println(F("ACK CMD ERROR: Unable to create JPEG on SD! END"));

        return;
      }
    }
  }

  length--;

  while (length--)
  {
    temp_last = temp;
    temp = SPI.transfer(0x00);

    if (IsHeader == true)
    {
      if (sendData)
      {
        if (PIN_SD_CS >= 0)
        {
          Camera.CS_HIGH();
          
          outputFile.write(&temp, 1);

          Camera.CS_LOW();
          Camera.set_fifo_burst();
        }
        else
        {
          Serial.write(temp);
        }
      }
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF))
    {
      IsHeader = true;

      if (sendData)
      {
        if (PIN_SD_CS >= 0)
        {
          Camera.CS_HIGH();

          outputFile.write(&temp_last, 1);
          outputFile.write(&temp, 1);

          Camera.CS_LOW();
          Camera.set_fifo_burst();
        }
        else
        {
          Serial.write(temp_last);
          Serial.write(temp);
        }
      }
    }

    if ((temp == 0xD9) && (temp_last == 0xFF))
      break;

    //delayMicroseconds(15);
  }

  Camera.CS_HIGH();
  Camera.clear_fifo_flag();

  if (sendData)
    if (PIN_SD_CS >= 0)
      outputFile.close();

  Serial.println(F("ACK CMD JPEG burst read! END"));

  IsHeader = false;

  return;
}*/

void TakePicture(int (*imageDataPtr)[320][240], bool sendData)
{
  Camera.flush_fifo();
  Camera.clear_fifo_flag();

  //if (SavingJPEG)
  //  Camera.OV2640_set_JPEG_size(OV2640_320x240);

  int currTime, captureTime, processingTime;

  Serial.println(F("ACK CMD Starting image capture... END"));

  Camera.start_capture();

  currTime = millis();

  while (!Camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

  captureTime = millis() - currTime;

  Serial.println(F("ACK CMD Image capture finished! END")); //TODO: Timing
  delay(50);

  Serial.println(F("ACK CMD Starting image processing... END"));

  currTime = millis();

  //if (SavingJPEG)
  //  ReadJPEGBurst(sendData);
  //else
    ReadBMPBurst(imageDataPtr, sendData);

  processingTime = millis() - currTime;
  
  Serial.println(F("ACK CMD Image processing finished! END")); //TODO: Timing
}

void InitCamera()
{
  #if !(defined OV2640_MINI_2MP)
    #error Camera OV2640 not selected!
  #endif

  Serial.println(F("ACK CMD Initializing camera... END"));

  Wire.begin();

  pinMode(PIN_CAMERA_CS, OUTPUT);
  digitalWrite(PIN_CAMERA_CS, HIGH);

  SPI.begin();

  Camera.write_reg(0x07, 0x80);
  delay(100);
  Camera.write_reg(0x07, 0x00);
  delay(100);

  uint8_t temp;

  while (1) 
  {
    Camera.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = Camera.read_reg(ARDUCHIP_TEST1);

    if (temp != 0x55)
    {
      Serial.println(F("ACK CMD ERROR: Camera SPI not setup properly! END"));
      delay(1000);
      
      continue;
    }
    else
    {
      break;
    }
  }

  uint8_t vid, pid;

  while (1)
  {
    Camera.wrSensorReg8_8(0xff, 0x01);
    Camera.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    Camera.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);

    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
    {
      Serial.println(F("ACK CMD ERROR: Camera OV2640 not detected! END"));
      delay(1000);
      
      continue;
    }
    else
    {
      break;
    } 
  }

  //if (SavingJPEG)
  //  Camera.set_format(JPEG);
  //else
    Camera.set_format(BMP);

  //Camera.OV2640_set_Special_effects(BW);

  Camera.InitCAM();
  
  //if (!SavingJPEG)
  //{
    Camera.wrSensorReg16_8(0x3818, 0x81);
    Camera.wrSensorReg16_8(0x3621, 0xA7);
  //}

  Serial.println(F("ACK CMD Successfully initialized camera! END"));
}