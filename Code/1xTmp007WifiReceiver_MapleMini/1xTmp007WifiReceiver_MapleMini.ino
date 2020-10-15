/*************************************************** 
  This is to be used with 1xTmp007WifiTransmitter sketch
  for transmitting/receiving TMP007 Barometric 
  Pressure & Temp Sensor data over wifi (nRF24L01)

  This code is for the receiver unit, which is  
  connected to the host PC via USB so it can display data 
  on the Serial Monitor. The transmitter unit is connected
  to the TMP007 sensor and streams data to the receiver.

  RF24 code modified from following communication tutorial:
          Arduino Wireless Communication Tutorial
                Example 1 - Receiver Code
                         
          by Dejan Nedelkovski, www.HowToMechatronics.com
          
          Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
 
  Author: Tom B.
  Date: 8/23/2019
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

//Default Serial Monitor printing to false - will be corrected to TRUE in setup() 
//if Serial Monitor is detected
boolean SERIALPRINT_ON = true;

//Define data struct
typedef struct
  {
    unsigned long tm;         //Time in milliseconds
    int IRtemp;               //IR sensed temp in F
    int Dietemp;              //Die temp in F
  } my_data_t;

//Create instance of my_data_t struct called data for data logging
    //Will be used more heavily for writing data to MicroSD card when that code 
    //is ported over from code for other configuration
my_data_t data;

//Create instances of SPIClass and RF24 objects
SPIClass SPI_2(2);
//RF24 radio(3,8); // CE, CSN 
RF24 radio(10,11); // CE, CSN

//Define address for radio
const byte address[6] = "00001";


//----Setup------------------------------------------------------------------

void setup() { 
  
  //Start Serial Monitor if it extsts
  if (Serial3) {
    Serial.begin(9600);
    SERIALPRINT_ON = true;
    //while(!Serial){};  // Wait until Serial is up and running
    Serial.println("Adafruit 1xTMP007 wifi example - receiver connected");
  }

  //Start and configure SPI
  SPI_2.begin();
  SPI.setModule(2);

  //Begin radio for listening
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

} //End of void setup() ------------------------------------------------------


//----Start Loop--------------------------------------------------------------

void loop() {
//Serial.println("Adafruit 1xTMP007 wifi example - receiver connected");
  //If radio transmission has been received, display to Serial Monitor
  if (radio.available()) {
  
    //Get current time
    data.tm = millis();

    //Define placeholder variable for received text then read message into it
    char text[90] = "";
    radio.read(&text, sizeof(text));

    //Slice temperatures into separate character variables;
    char objTempChar[90] = "";
    char dieTempChar[90] = "";
    memcpy(objTempChar,text+4,3);
    memcpy(dieTempChar,text+12,3);

    //Parse message and read in to 
    data.IRtemp = atoi(objTempChar);//toInt(objTempChar);
    data.Dietemp = atoi(dieTempChar);//toInt(dieTempChar);

    //Print raw and interpreted data to Serial Monitor if Serial is connected
    if (SERIALPRINT_ON) {
      Serial.print("Object Temperature: "); Serial.print(data.IRtemp); Serial.println("*F");
      Serial.print("Die Temperature:    "); Serial.print(data.Dietemp); Serial.println("*F");
    }

    //Future writing to MicroSD card will take place here, using "data" struct
  
  }
  //else {Serial.print("No msg available\n");}
  
} //End of void loop() ------------------------------------------------------
