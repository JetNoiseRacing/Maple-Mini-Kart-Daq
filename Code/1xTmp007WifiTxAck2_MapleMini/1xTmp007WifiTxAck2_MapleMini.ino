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

//Include required libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include "Adafruit_TMP007.h"

#define CE_PIN 10
#define CSN_PIN 11

//Default Serial Monitor printing to false - will be corrected to TRUE in setup() 
//if Serial Monitor is detected
boolean SERIALPRINT_ON = false;  

//Create instances of SPIClass and RF24
SPIClass SPI_2(2);
RF24 radio(CE_PIN, CSN_PIN); // CE, CSN

const byte address[6] = "00002";  //Define address for radio

Adafruit_TMP007 tmp007(0x40); //Create instance of IR temp sensor object ...
                              //at nominal I2C address

typedef struct //Define data struct for temperatures and counter
  {
    int count = 1;  //Counter to indicate if a frame has been dropped
    int objTemp;    //Object IR sensed temp in F
    int dieTemp;    //Die temp in F
  } IR_data;

IR_data data;  //Create instance of IR_data struct called data for data logging

int ackIRtempData[3] = {0,1,2}; //{Counter, Object Temp, Die Temp} ... this is the data
                                //sent back to the central controller when requested

char requestMsgRec[10]; //request message received from central controller
                        //must match requestMsgSent in the central controller
bool newRequest = false; //has a new request for temperature data been received...
                         //from the central controller?

//----Setup------------------------------------------------------------------

void setup() {

  //Start Serial Monitor if it exists
  if (Serial) {
    Serial.begin(9600);
    SERIALPRINT_ON = true;
    while(!Serial){};  // Wait until Serial is up and running
    Serial.println("IR temp over wifi example running...");
  }

  //Setup IR sensor #1 - stop if no IR sensor found
      // Can also use tmp007.begin(TMP007_CFG_1SAMPLE) or 2SAMPLE/4SAMPLE/8SAMPLE to have
      // lower precision, higher rate sampling. Default is TMP007_CFG_16SAMPLE - takes
      // 4 seconds per reading (16 samples)
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

  //Start and configure SPI
  SPI_2.begin();
  SPI.setModule(2);

  //Begin radio for transmitting and receiving
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(1,address);
  //radio.setPALevel(RF24_PA_MIN);
  radio.enableAckPayload();
  radio.startListening();

  //Set up built-in LED for use in indicating when messages are transmitted
  pinMode(LED_BUILTIN, OUTPUT);
  
}//End of void setup() ------------------------------------------------------


//----Start Loop--------------------------------------------------------------

void loop() {

  newRequest = false; //reset newRequest to false

  //Read object and die temps, convert from C to F
  //Use integer variable types for processing speed
  data.objTemp = int(tmp007.readObjTempC())*9/5+32;
  data.dieTemp = int(tmp007.readDieTempC())*9/5+32;

  ackIRtempData[0] = data.count;
  ackIRtempData[1] = data.objTemp;
  ackIRtempData[2] = data.dieTemp;

  radio.writeAckPayload(1,&ackIRtempData,sizeof(ackIRtempData)); //pre-load IR temp data

  if ( radio.available() ) {
    radio.read(&requestMsgRec,sizeof(requestMsgRec));
    newRequest = true;
    data.count = data.count+1;
  }
  
/*
  //Turn on built-in LED to indicate message being transmitted, then initiate transmission
  digitalWrite(LED_BUILTIN, HIGH);
  
  //Print transmitted message to Serial Monitor if connected
  if (Serial) {
    Serial.println(text);
  }
  
  //Delay 250ms so transmission light is visible
  delay(250);

  //Turn off transmission indication and wait 750ms for next IR sensor reading
  digitalWrite(LED_BUILTIN, LOW);
  delay(750);
*/
} //End of void loop() ------------------------------------------------------
