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

#include <Wire.h>                               //Include required libraries
#include <SD.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN 10                               //Define CE pin for RF24 radio
#define CSN_PIN 11                              //Define CSN pin for RF24 radio

boolean SERIALPRINT_ON = false;                 //Default serial print to false, set true later if Serial exists

typedef struct                                  //Create struct definition for IR sensors
  {
    float tm      = 0.0;                        //Time in seconds
    int count     = 0;                          //count indication from sensor unit
    int IRtemp    = 0;                          //IR sensed temp in F
    int Dietemp   = 0;                          //Die temp in F
    int fault     = 0;                          //Fault flag 
  } IRsensDEF;                                  //    0: No fault
                                                //    1: AckPayload not available
                                                //    2: Unsuccessful TX
                                               
central_data_struct data;                       //Create instance of central struct for data logging

SPIClass SPI_2(2);                              //Set SPI #2
RF24 radio(CE_PIN,CSN_PIN);                     //Create instance of RF24 radio

const byte numSens = 2;                         //Number of remote sensors with which hub communcates

typedef struct                                  //Define central struct as struct of sensor structs
  {
    IRsensDEF IRsens[2]; 
  } central_data_struct;
  
const byte addresses[numSens][6] = {"00001","00002"}; //Define addresses for radio

char requestMsg[10] = "ToSenN  0";              //Compose main body of message for AckPayload message
char msgNum = '0';                              //Variable for incrementing AckPayload message number

int ackIRtempData[3] = {0,0,0};                 //Vector to contain data received from sensors

unsigned long currentMillis = 0;                //Variables for polling sensors at 1Hz
unsigned long prevMillis = 0;
unsigned long intervMillis = 1000;


//---- Start of void setup() -------------------------------------------------
void setup() { 
  
  if (Serial) {                                 //Start Serial Monitor if it extsts
    Serial.begin(9600);
    SERIALPRINT_ON = true;
  }

  SPI_2.begin();                                //Start SPI #2 for RF24 radio
  SPI.setModule(2);                             //Set SPI focus to SPI #2 - Maple Mini-unique function
 
  radio.begin();                                //Begin radio for listening and set desired parameters
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.setPALevel(RF24_PA_HIGH);
  radio.setRetries(5,5);                        //(delay=5ms,count=5)

} //End of void setup() ------------------------------------------------------


//----Start Loop--------------------------------------------------------------
void loop() {

  currentMillis = millis();                     //Get time

  if (currentMillis-prevMillis>=intervMillis) { //Get IR temperatures in 1-second intervals
    for (int n=0; n<numSens; n++) {
      
      radio.openWritingPipe(addresses[n]);      //Change radio pipe to sensor of focus in this iteration
      
      requestMsg[5] = n+1+'0';                  //Increment Ackpayload message number

      bool successTX;

      successTX = radio.write(&requestMsg, sizeof(requestMsg)); //Send request to sensor in focus

      if (successTX) {
        if (radio.isAckPayloadAvailable()) {                    //Store data in struct if AckPayload received
          radio.read(&ackIRtempData, sizeof(ackIRtempData));
          data.IRsens[n].tm = currentMillis/1000.0;
          data.IRsens[n].count = ackIRtempData[0];
          data.IRsens[n].IRtemp = ackIRtempData[1];
          data.IRsens[n].Dietemp = ackIRtempData[2];
          data.IRsens[n].fault = 0;
        }
        else {
          data.IRsens[n].tm = currentMillis/1000.0;             //Set no AckPayload fault flag and keep 
          data.IRsens[n].fault = 1;                             //previous frame data
          //count, IRtemp, and Dietemp kept from previous loop to avoid discontenuities
        }
      }
      else {
        data.IRsens[n].tm = currentMillis/1000.0;               //Set TX unsuccessful fault flag and keep 
        data.IRsens[n].fault = 2;                               //previous frame data
        //count, IRtemp, and Dietemp kept from previous loop to avoid discontenuities
      }
    }

    if (Serial) {                                               //Print data received from sensors to Serial
      Serial.println("IR Temp Sensor #1\t\tIR Temp Sensor #2"); 
      char buftm[50],buf1[50],buf2[50];

      if (data.IRsens[0].fault==1) {
        Serial.println("Sensor #1 AckPayload FAILED");
      }
      else if (data.IRsens[0].fault==2) {
        Serial.println("Sensor #1 TX FAILED");
      }
      
      if (data.IRsens[1].fault==1) {
        Serial.println("Sensor #2 AckPayload FAILED");
      }
      else if (data.IRsens[1].fault==2) {
        Serial.println("Sensor #2 TX FAILED");
      }

      //Assemble printed string using sprintf
      sprintf(buftm,"Time (s): %6.3f\t\tTime (s): %6.3f",data.IRsens[0].tm,data.IRsens[1].tm);
      Serial.println(buftm); //+ buf1 + "\t\t\tTime (s): " + buf2);

      sprintf(buf1,"Count: %i\t\t\tCount: %i",data.IRsens[0].count,data.IRsens[1].count);
      Serial.println(buf1);

      sprintf(buf1,"Obj Temp (F): %i\t\tObj Temp (F): %i",data.IRsens[0].IRtemp,data.IRsens[1].IRtemp);
      Serial.println(buf1);

      sprintf(buf1,"Die Temp (F): %i\t\tDie Temp(F): %i\n",data.IRsens[0].Dietemp,data.IRsens[1].Dietemp);
      Serial.println(buf1);
      
      //Assemble printed string using itoa      
      /*itoa(data.IRsens[0].count,buf1,10); itoa(data.IRsens[1].count,buf2,10); 
      Serial.println(String("Count: ") + buf1 + "\t\t\tCount: " + buf2);
      itoa(data.IRsens[0].IRtemp,buf1,10); itoa(data.IRsens[1].IRtemp,buf2,10); 
      Serial.println(String("IRtemp (F): ") + buf1 + "\t\t\tIRTemp (F): " + buf2);
      itoa(data.IRsens[0].Dietemp,buf1,10); itoa(data.IRsens[1].Dietemp,buf2,10);
      Serial.println(String("Dietemp (F): ") + buf1 + "\t\t\tDietemp (F): " + buf2);
      Serial.print('\n');*/
    }
    
    msgNum+=1;                                    //Increment AckPayload message number,
    if (msgNum>'9') {                             //keeping between 1 and 9
      msgNum = '0';
    }
    requestMsg[8] = msgNum;                       //Update AckPayload message with new message number
    prevMillis = currentMillis;                   //Reset timestamp of previous transmission
  }
}




