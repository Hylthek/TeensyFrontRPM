//global scope=================================================================================================
//header files-------------------------------------------------------------------------------------------------
#include <string>
#include "SdFat.h"
#include <SD.h>
#include <SPI.h>
//changable const variables and objects------------------------------------------------------------------------
File deviceFiles[2]; //{LeftRPM_data,  RightRPM_data}
std::string fileNames[2] = {"LeftRPM_data", "RightRPM_data"};
std::string fileHeaders[2] = {
  "Timestamp,Milliseconds,RPM\n" ,
  "Timestamp,Milliseconds,RPM\n"
};
//consts-------------------------------------------------------------------------------------------------------
const int pinLED = 13; //for debugging
const int pinRPM[2] = {0,1}; //hall-effect input
const int stuffingCutoff = 1000;
//global variables---------------------------------------------------------------------------------------------
bool prevSensorState[2] = {0, 0}; //previous state of the rpm sensor
unsigned long prevMilliseconds[2] = {0, 0}; //time since last sample (mSec)
unsigned long prevMicroseconds[2] = {0, 0}; //time since last sample (uSec)
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
  for (int i = 0; i < 2; i++) {
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
  pinMode(pinRPM[0], INPUT_PULLUP);
  pinMode(pinRPM[1], INPUT_PULLUP);
  //add data titles
  for(int i = 0; i < 2; i++) {
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
  for(int i = 0; i < 2; i++) {
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