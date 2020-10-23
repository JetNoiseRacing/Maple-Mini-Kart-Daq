/*************************************************** 
  This is to be used with 2xTmp007WifiRxAck_MapleMini sketch
  for transmitting/receiving TMP007 Barometric 
  Pressure & Temp Sensor data over wifi (nRF24L01)

  This code is for the transmitter unit #2, which is connected
  to the TMP007 sensor and streams data to the receiver
  when polled by the central transceiver.

  RF24 code modified from following sources:
        1. https://forum.arduino.cc/index.php?topic=421081.0
        2. Library: TMRh20/RF24, https://github.com/tmrh20/RF24/

  Author: Tom B.
  Date: 9/26/2020      
***************************************************/

#include <SPI.h>                                //Include required libraries
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include "Adafruit_TMP007.h"

#define CE_PIN 10                               //Define CE pin for RF24 radio
#define CSN_PIN 11                              //Define CSN pin for RF24 radio

boolean SERIALPRINT_ON = false;                 //Default serial print to false, set true later if Serial exists 

typedef struct                                  //Create struct definition for IR sensors
  {
    int count = 0;                              //0-9 Counter to indicate if a frame has been dropped
    int objTemp;                                //Object IR sensed temp in F
    int dieTemp;                                //Die temp in F
  } IR_data;

IR_data data;                                   //Create instance of IR_data struct to hold data
  
Adafruit_TMP007 tmp007(0x40);                   //Create instance of IR temp sensor object ...
                                                //at nominal I2C address

SPIClass SPI_2(2);                              //Set SPI #2
RF24 radio(CE_PIN, CSN_PIN);                    //Create RF24 radio object

const byte address[6] = "00002";                //Define address for radio #2

int ackIRtempData[3] = {0,0,0};                 //{Counter, Object Temp, Die Temp} - this is the data
                                                //sent back to the central controller when requested

char requestMsgRec[10];                         //Holder for request message received from central controller

bool newRequest = false;                        //Flag for new request received from central controller

unsigned long currentMillis = 0;                //Will poll temp sensor and update AckPayload at 10Hz
unsigned long prevMillis = 0;                   //NOTE: In this application AckPayload starts uploading 
unsigned long intervMillis = 100;               //      erroneous data if you writeAckPayload() faster 
                                                //      than ~10Hz

//----Setup------------------------------------------------------------------
void setup() {

  if (Serial) {                                 //Start Serial Monitor if it exists
    Serial.begin(9600);
    SERIALPRINT_ON = true;
  }

  //Setup IR sensor #1 - stop if no IR sensor found
  //   Can also use tmp007.begin(TMP007_CFG_1SAMPLE) or 2SAMPLE/4SAMPLE/8SAMPLE to have
  //   lower precision, higher rate sampling. Default is TMP007_CFG_16SAMPLE - takes
  //   4 seconds per reading (16 samples)
  if (! tmp007.begin(TMP007_CFG_4SAMPLE)) {
    if (SERIALPRINT_ON) {
      Serial.println("No IR sensor found");
    }
    while (1);
  }
  else {
    if (SERIALPRINT_ON) {
      Serial.println("IR sensor found!");
    }
  }

  SPI_2.begin();                                //Start SPI #2 for RF24 radio
  SPI.setModule(2);                             //Set SPI focus to SPI #2 - Maple Mini-unique function

  radio.begin();                                //Begin radio for listening and set desired parameters
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(1,address);
  radio.setPALevel(RF24_PA_HIGH);
  radio.enableAckPayload();
  radio.startListening();                       
  
}//End of void setup() ------------------------------------------------------


//----Start Loop--------------------------------------------------------------

void loop() {

  newRequest = false;                                   //Reset newRequest to false

  currentMillis = millis();                             //Get time

  if (currentMillis-prevMillis>=intervMillis) {         //Get IR temperatures in 100ms intervals
          
    data.objTemp = int(tmp007.readObjTempC())*9/5+32;   //Read object and die temps, convert from C to F
    data.dieTemp = int(tmp007.readDieTempC())*9/5+32;   //Use integer variable types for processing speed 
  
    ackIRtempData[0] = data.count;                      //Put IR data and count into vector for AckPreload
    ackIRtempData[1] = data.objTemp;
    ackIRtempData[2] = data.dieTemp;
    
    radio.writeAckPayload(1,&ackIRtempData,sizeof(ackIRtempData)); //Pre-load IR temp data

    prevMillis = currentMillis;                         //Reset timestamp of previous AckPayload update
  }
  
  if ( radio.available() ) {                            
    radio.read(&requestMsgRec,sizeof(requestMsgRec));   //Store message from central controller (currently unused)
    newRequest = true;
    if (data.count>=9) {                                //Increment data message counter
      data.count = 0;
    }
    else {
      data.count = data.count+1;
    }
  }
} //End of void loop() ------------------------------------------------------
