//global scope=================================================================================================
//header files-------------------------------------------------------------------------------------------------
#include <string>
#include "SdFat.h"
#include <SD.h>
#include <SPI.h>
//changable const variables and objects------------------------------------------------------------------------
const int numDevices = 3;
File deviceFiles[numDevices]; //{LeftRPM_data,  RightRPM_data, rearRPM_data}
std::string fileNames[numDevices] = {"frontLeftRPM_data", "frontRightRPM_data", "RearRPM_data"};
std::string fileHeaders[numDevices] = {
  "Timestamp,Milliseconds,frontLeftRPM\n" ,
  "Timestamp,Milliseconds,frontRightRPM\n",
  "Timestamp,Milliseconds,RearRPM\n"
};
//consts-------------------------------------------------------------------------------------------------------
const int pinLED = 13; //for debugging
const int pinRPM[numDevices] = {0,1,2}; //hall-effect input {0=left, 1=right, 2=rear}
const int stuffingCutoff = 1000;
//global variables---------------------------------------------------------------------------------------------
bool prevSensorState[numDevices] = {0, 0, 0}; //previous state of the rpm sensor
unsigned long prevMilliseconds[numDevices] = {0, 0, 0}; //time since last sample (mSec)
unsigned long prevMicroseconds[numDevices] = {0, 0, 0}; //time since last sample (uSec)
unsigned long setupTimeOffset = 0; //offsets time so setup time is ignored in output file
//SdExFat sd;       //IDK delete? (it works without it, so...)
//SdVolume volume;  //IDK delete? (it works without it, so...)
Sd2Card card;
//setup function===============================================================================================
void setup() { // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(pinLED, OUTPUT);
  //setup microSD card-----------------------------------------------------------------------------------------
  if (SD.begin(BUILTIN_SDCARD) == false) {
    while (true) { //blink indefinitely
      digitalWrite(pinLED, HIGH);
      delay(333);
      digitalWrite(pinLED, LOW);
      delay(333);
    }
  }
  if (card.init(SPI_HALF_SPEED, BUILTIN_SDCARD) == false) {
    while (true) { //blink indefinitely
      digitalWrite(pinLED, HIGH);
      delay(333);
      digitalWrite(pinLED, LOW);
      delay(333);
    }
  }
  //file creation----------------------------------------------------------------------------------------------
  for (int i = 0; i < numDevices; i++) {
    int currFileNum = 1;
    std::string currFileName = fileNames[i];
    while (true) {
      currFileName += '_';
      currFileName += std::to_string(currFileNum);
      currFileName += ".csv";
      if (SD.exists(currFileName.data())) {
        currFileName = fileNames[i];
        currFileNum++;
        continue;
      }
      else {
        deviceFiles[i] = SD.open(currFileName.data(), O_CREAT | O_TRUNC | O_WRITE);
        break;
      }
    }
  }
  //setup i/o pins and serial object---------------------------------------------------------------------------
  for(int i = 0; i < numDevices; i++) {
    pinMode(pinRPM[i], INPUT_PULLUP);
  }
  //add data titles
  for(int i = 0; i < numDevices; i++) {
    deviceFiles[i].print(fileHeaders[i].data());
  }
  //post-setup-------------------------------------------------------------------------------------------------
  for (int i = 0; i < 30; i++) {
    digitalWrite(pinLED, HIGH);
    delay(50);
    digitalWrite(pinLED, LOW);
    delay(50);
  }
  setupTimeOffset = millis();
}
//loop function================================================================================================
void loop() { // put your main code here, to run repeatedly:
  for(int i = 0; i < numDevices; i++) {
    //set "curr" variables---------------------------------------------------------------------------------------
    unsigned long currMilliseconds = millis();
    unsigned long currMicroseconds = micros();
    bool currSensorState = digitalRead(pinRPM[i]);
    //calculate RPM----------------------------------------------------------------------------------------------
    if (currSensorState != prevSensorState[i]) {
      prevSensorState[i] = digitalRead(pinRPM[i]);
      if(currSensorState == LOW) {
        //calculate- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        unsigned long x = currMicroseconds - prevMicroseconds[i];
        unsigned long RPM = (60000000 / (x * 12)); // RpM = 60,000,000/x*12
        //log array to SD card- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        deviceFiles[i].print(millisToTimestamp(currMilliseconds-setupTimeOffset).data());
        deviceFiles[i].print(",");
        deviceFiles[i].print(currMilliseconds-setupTimeOffset);
        deviceFiles[i].print(",");
        deviceFiles[i].print(RPM);
        deviceFiles[i].print("\n");

        Serial.print(millisToTimestamp(currMilliseconds-setupTimeOffset).data());
        Serial.print(",");
        Serial.print(currMilliseconds-setupTimeOffset);
        Serial.print(",");
        Serial.print(RPM);
        Serial.print("\n");
        
        //update "prev" variables - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        prevMilliseconds[i] = millis();
        prevMicroseconds[i] = micros();
      }
    }
    //zero stuffing----------------------------------------------------------------------------------------------
    else if (currMilliseconds - prevMilliseconds[i] > stuffingCutoff) {
      //log array to SD card- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        deviceFiles[i].print(millisToTimestamp(currMilliseconds-setupTimeOffset).data());
        deviceFiles[i].print(",");
        deviceFiles[i].print(currMilliseconds-setupTimeOffset);
        deviceFiles[i].print(",");
        deviceFiles[i].print("0");
        deviceFiles[i].print("\n");

        Serial.print(millisToTimestamp(currMilliseconds-setupTimeOffset).data());
        Serial.print(",");
        Serial.print(currMilliseconds-setupTimeOffset);
        Serial.print(",");
        Serial.print("0");
        Serial.print("\n");
        
      //update prevMilliseconds - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      prevMilliseconds[i] = millis();
    }
    //save data--------------------------------------------------------------------------------------------------
    deviceFiles[i].flush();
  }
}

std::string millisToTimestamp(unsigned long millisParam) {
  //calculate values
  int millisecond = millisParam % 1000;
  int second = (millisParam / 1000) % 60;
  int minute = (millisParam / (60*1000) ) % 60;
  int hour = millisParam / (60*60*1000);
  //make output string
  std::string output = "";
  output += std::to_string(hour);
  output += " ";
  if(minute < 10) //align every number
    output += "0";
  output += std::to_string(minute);
  output += " ";
  if(second < 10) //align every number
    output += "0";
  output += std::to_string(second);
  output += " ";
  if(millisecond < 10) //align every number
    output += "0";
  if(millisecond < 100) //align every number
    output += "0";
  output += std::to_string(millisecond);
  return output;
}