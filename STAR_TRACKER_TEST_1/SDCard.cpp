#include "SDCard.h"

const int PIN_SD_CS = 8;

void InitSDCard()
{
  if (PIN_SD_CS < 0)
    return;

  Serial.println(F("ACK CMD Initializing SD card... END"));

  while (!SD.begin(PIN_SD_CS))
  {
    Serial.println(F("ACK CMD ERROR: Unable to initialize SD card! END"));
    delay(1000);
  }

  while (1)
  {
    String testFileName = "TEST.txt";
    char buff[100];
    
    testFileName.toCharArray(buff, testFileName.length() + 1);
    buff[testFileName.length()] = '\0';

    File testFile = SD.open(buff, FILE_WRITE);

    if (!testFile)
    {
      Serial.println(F("ACK CMD ERROR: Unable to create files on SD card! END"));
      delay(1000);

      continue;
    }

    SD.remove(buff);

    break;
  }

  Serial.println(F("ACK CMD Successfully initialized SD card! END"));
}