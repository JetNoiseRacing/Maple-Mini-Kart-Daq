/*************************************************** 
  This is to be used with 1xTmp007WifiReceiver sketch
  for transmitting/receiving TMP007 Barometric 
  Pressure & Temp Sensor data over wifi (nRF24L01)

  This code is for the transmitter unit, which is connected
  to the TMP007 sensor and streams data to the receiver.
  The receiver is connected to the host PC via USB so 
  it can display data on the Serial Monitor. 

  RF24 code modified from following communication tutorial:
        Arduino Wireless Communication Tutorial
            Example 1 - Transmitter Code
                      
        by Dejan Nedelkovski, www.HowToMechatronics.com
       
        Library: TMRh20/RF24, https://github.com/tmrh20/RF24/

  Author: Tom B.
  Date: 8/24/2019      
***************************************************/

//Include required libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include "Adafruit_TMP007.h"

//Default Serial Monitor printing to false - will be corrected to TRUE in setup() 
//if Serial Monitor is detected
boolean SERIALPRINT_ON = false;

//TMP007 pin connections as follows for Arduino Mega:
    // Digital pin #21: SCL
    // Digital pin #20: SDA   

//Create instances of SPIClass and RF24 objects
RF24 radio(7, 8); // CE, CSN

//Define address for radio
const byte address[6] = "00001";

//Create instance of IR temp sensor object at nominal I2C address
Adafruit_TMP007 tmp007(0x40);

//Define data struct for temperatures and counter
typedef struct
  {
    int count = 1;  //Counter to indicate if a frame has been dropped
    int objTemp;    //Object IR sensed temp in F
    int dieTemp;    //Die temp in F
  } IR_data;

//Create instance of IR_data struct called data for data logging
IR_data data;

//----Setup------------------------------------------------------------------

void setup() {

  //Start Serial Monitor if it exists
  if (Serial) {
    Serial.begin(9600);
    SERIALPRINT_ON = true;
    while(!Serial){};  // Wait until Serial is up and running
    Serial.println("Adafruit 1xTMP007 wifi example - transmitter connected");
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

  //Begin radio for transmitting (receiveing/listening not needed, so stop it)
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();

  //Set up built-in LED for use in indicating when messages are transmitted
  pinMode(LED_BUILTIN, OUTPUT);
  
}//End of void setup() ------------------------------------------------------


//----Start Loop--------------------------------------------------------------

void loop() {

  //Read object and die temps, convert from C to F
  //Use integer variable types for processing speed
  data.objTemp = int(tmp007.readObjTempC())*9/5+32;
  data.dieTemp = int(tmp007.readDieTempC())*9/5+32;
  
  //Create preallocated char variable for message to be transmitted
  char text[90];
  sprintf(text,"Obj=%3i,Die=%3i,Cnt=%5i,",data.objTemp,data.dieTemp,data.count);

  //Turn on built-in LED to indicate message being transmitted, then initiate transmission
  digitalWrite(LED_BUILTIN, HIGH);
  radio.write(&text, sizeof(text));

  data.count=data.count+1; //Increment counter
  
  //Print transmitted message to Serial Monitor if connected
  if (Serial) {
    Serial.println(text);
  }
  
  //Delay 250ms so transmission light is visible
  delay(250);

  //Turn off transmission indication and wait 750ms for next IR sensor reading
  digitalWrite(LED_BUILTIN, LOW);
  delay(750);
  
} //End of void loop() ------------------------------------------------------
