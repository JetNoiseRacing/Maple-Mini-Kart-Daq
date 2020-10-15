/*************************************************** 
  This is to be used with 1xTmp007WifiTxAck1/2_MapleMini sketches
  for transmitting/receiving TMP007 Barometric 
  Pressure & Temp Sensor data over wifi (nRF24L01)

  This code is for the central controller, which received IR
  temperature data from the two sensing units.

  RF24 code modified from following sources:
        1. https://forum.arduino.cc/index.php?topic=421081.0
        2. Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
 
  Author: Tom B.
  Date: 9/27/2020
 ****************************************************/

//Include required libraries
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//Define I2C communication pins
#define SS          7
#define SCK         6
#define MISO        5
#define MOSI        4

#define CE_PIN 10
#define CSN_PIN 11

//Default Serial Monitor printing to false - will be corrected to TRUE in setup() 
//if Serial Monitor is detected
boolean SERIALPRINT_ON = true;

//Create struct definition for IR sensors
typedef struct
  {
    unsigned long tm = 1024;         //Time in milliseconds
    int count;                //count indication from sensor unit
    int IRtemp;               //IR sensed temp in F
    int Dietemp;              //Die temp in F
  } IRsensDEF;

//Define master central data struct as a struct of the various sensor structs
typedef struct
  {
    IRsensDEF IRsens[2]; 
  } central_data_struct;

//Create instance of central_data_struct struct called data for data logging
    //Will be used more heavily for writing data to MicroSD card when that code 
    //is ported over from code for other configuration
central_data_struct data;

//Create instances of SPIClass and RF24 objects
SPIClass SPI_2(2);
RF24 radio(CE_PIN,CSN_PIN); // CE, CSN

const byte numSens = 2;

//Define addresses for radio
const byte addresses[numSens][6] = {"00001","00002"};

char requestMsg[10] = "ToSenN  0"; 
char msgNum = '0';

int ackIRtempData[3] = {0,0,0};

unsigned long currentMillis = 0;
unsigned long prevMillis = 0;
unsigned long intervMillis = 1000;


//----Setup------------------------------------------------------------------

void setup() { 
  
  //Start Serial Monitor if it extsts
  if (Serial) {
    Serial.begin(9600);
    SERIALPRINT_ON = true;
    while(!Serial){};  // Wait until Serial is up and running
    Serial.println("Adafruit 1xTMP007 wifi example - receiver connected");
  }

  //Start and configure SPI
  SPI_2.begin();
  SPI.setModule(2);

  //Begin radio for listening
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.setRetries(3,5); //delay,count

} //End of void setup() ------------------------------------------------------


//----Start Loop--------------------------------------------------------------

void loop() {

  // Get time
  currentMillis = millis();

  // Get IR temperatures
  if (currentMillis-prevMillis>=intervMillis) {
    for (int n=0; n<numSens; n++) {
      radio.openWritingPipe(addresses[n]);

      requestMsg[5] = n+1+'0';

      bool successTX;

      successTX = radio.write(&requestMsg, sizeof(requestMsg));

      if (successTX) {
        if (radio.isAckPayloadAvailable()) {
          radio.read(&ackIRtempData, sizeof(ackIRtempData));
          data.IRsens[n].tm = currentMillis;
          data.IRsens[n].count = ackIRtempData[0];
          data.IRsens[n].IRtemp = ackIRtempData[1];
          data.IRsens[n].Dietemp = ackIRtempData[2];
        }
        else {
          data.IRsens[n].tm = currentMillis;
          data.IRsens[n].count = 999;
          data.IRsens[n].IRtemp = 999;
          data.IRsens[n].Dietemp = 999;
        }
      }
      
    }
    msgNum+=1;
    if (msgNum>'9') {
      msgNum = '0';
    }
    requestMsg[8] = msgNum;
    prevMillis = currentMillis;
  }
}




