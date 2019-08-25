/*************************************************** 
  This code reads temperatures from 2 TMP007 devices
  and saves data to MicroSD card. Data recording is
  controlled by physical start/stop recording switch.
  Device then illuminates LEDs (blue/green/red) based
  on measured TMP007 #1 temperature relative to thresholds - 
  meant to simulate monitoring cold/optimal/hot kart
  tires temperatures throughout a race.
  
  Code leverages some material from following Adafruit example:
        This is an example for the TMP007 Barometric Pressure & Temp Sensor
      
        Designed specifically to work with the Adafruit TMP007 Breakout 
        ----> https://www.adafruit.com/products/2023
        
        These displays use I2C to communicate, 2 pins are required to interface
        Adafruit invests time and resources providing this open source code, 
        please support Adafruit and open-source hardware by purchasing 
        products from Adafruit!
      
        Written by Limor Fried/Ladyada for Adafruit Industries.  
        BSD license, all text above must be included in any redistribution

 Author: Tom B.
        
 ****************************************************/

//Specify desired name for SD card data file
String SDFilename = "MMTMP15.txt";

//Include required libraries 
#include <Wire.h>
#include "Adafruit_TMP007_1.8.5.h"
#include <SD.h>

//Define pin location of LEDs and temperature thresholds
#define BLUE_LED    12
#define GREEN_LED   13
#define RED_LED     14
#define BLUE_THRESH 70
#define RED_THRESH  80

//Define pins used for writing to MicroSD card
#define SS          7
#define SCK         6
#define MISO        5
#define MOSI        4
#define ChipDet     3

//Define pins used for data recording start/stop switch
#define DAcqHi      17
#define DAcqSense   18

//Default Serial Monitor printing to false - will be corrected to TRUE in setup() 
//if Serial Monitor is detected
boolean SERIALPRINT_ON = false;

//TMP007 pin connections as follows for Arduino Mega:
    // Digital pin #21: SCL
    // Digital pin #20: SDA   

//Set up TMP007 objects
Adafruit_TMP007 tmp007_1(0x40);  // Sensor #1
Adafruit_TMP007 tmp007_2(0x42);  // Sensor #2

//Set file handle for MicroSD outfile
File myFile;

//Specify digital pin for chipselect
const int chipSelect = 7;

//Initialize MicroSD file control variables
bool myFileClosed = false;
bool myFileExists = false;
bool SDinitFailed = false;
int SDcardExists = 0;
int DataAcqSwON = 0;

//Create data struct for temperature data
typedef struct
  {
    unsigned long tm;         //Time in milliseconds
    int IRtemp1;    //IR #1 sensed temp in F
    int Dietemp1;   //Die temp #1 in F
    int IRtemp2;    //IR #2 sensed temp in F
    int Dietemp2;   //Die temp #2 in F
  } my_data_t;

//Create instance of my_data_t struct called data for data logging
my_data_t data;


//----Setup----------------------------------------------------------------------------

void setup() { 

  Wire.setClock(50000);  //Set I2C communication to 50kbaud for 5m max wire transmission distance

  //Start Serial Monitor if it exists
  if (Serial3) {
    Serial.begin(9600);
    SERIALPRINT_ON = true;
    while(!Serial){};  // Wait until Serial is up and running
    Serial.println("Adafruit 2xTMP007 example");
  }
  
  //Setup IR sensor #1 - stop if IR sensor not found
      // Can also use tmp007.begin(TMP007_CFG_1SAMPLE) or 2SAMPLE/4SAMPLE/8SAMPLE to have
      // lower precision, higher rate sampling. Default is TMP007_CFG_16SAMPLE - takes
      // 4 seconds per reading (16 samples)
  if (! tmp007_1.begin(TMP007_CFG_4SAMPLE)) {
    if (SERIALPRINT_ON) {
      Serial.println("No IR sensor #1 found");
    }
    while (1);
  }
  else {
    if (SERIALPRINT_ON) {
      Serial.println("IR sensor #1 found!");
    }
  }

  //Setup IR sensor #2 - stop if IR sensor not found
  if (! tmp007_2.begin(TMP007_CFG_4SAMPLE)) {
    if (SERIALPRINT_ON) {
      Serial.println("No IR sensor #2 found");
    }
    while (1);
  }
  else {
    if (SERIALPRINT_ON) {
      Serial.println("IR sensor #2 found!");
    }
  }

  //Define LED pins for voltage output
  pinMode(BLUE_LED,OUTPUT);
  pinMode(GREEN_LED,OUTPUT);
  pinMode(RED_LED,OUTPUT);

  //Define MicroSD control pins
  pinMode(ChipDet,INPUT);
  pinMode(DAcqHi,OUTPUT);
  pinMode(DAcqSense,INPUT);

  //Set data acq pin to HIGH - if switch is closed, this will close circuit 
  //setting DAcqSense to HIGH as well. When DAcqSense is HIGH, data will be 
  //recorded (recording off when LOW).
  digitalWrite(DAcqHi,HIGH);

}//End of void setup() ------------------------------------------------------


//----Start Loop--------------------------------------------------------------

void loop() {

   //Get current time
   data.tm = millis();

   //Read data from TMP007 devices
   data.IRtemp1 = int(tmp007_1.readObjTempC())*9/5+32;
   data.Dietemp1 = int(tmp007_1.readDieTempC())*9/5+32;
   data.IRtemp2 = int(tmp007_2.readObjTempC())*9/5+32;
   data.Dietemp2 = int(tmp007_2.readDieTempC())*9/5+32;

   //Print data to screen if Serial Monitor is connected
   if (SERIALPRINT_ON) {
     Serial.print("Object #1 Temperature: "); Serial.print(data.IRtemp1); Serial.println("*F");
     Serial.print("Die #1 Temperature:    "); Serial.print(data.Dietemp1); Serial.println("*F");
     Serial.print("Object #2 Temperature: "); Serial.print(data.IRtemp2); Serial.println("*F");
     Serial.print("Die #2 Temperature:    "); Serial.print(data.Dietemp2); Serial.println("*F");
   }

   //Illuminate proper LED based on TMP007 #1 object measurement
   if (data.IRtemp1<BLUE_THRESH) {
     digitalWrite(BLUE_LED,HIGH);
     digitalWrite(GREEN_LED,LOW);
     digitalWrite(RED_LED,LOW);
   }
   else if (data.IRtemp1>RED_THRESH) {
     digitalWrite(BLUE_LED,LOW);
     digitalWrite(GREEN_LED,LOW);
     digitalWrite(RED_LED,HIGH);
   }
   else {
     digitalWrite(BLUE_LED,LOW);
     digitalWrite(GREEN_LED,HIGH);
     digitalWrite(RED_LED,LOW);
   }

   //For monitoring SD card status - display if it is present
   SDcardExists = digitalRead(ChipDet);
   if (SDcardExists && SERIALPRINT_ON) {
     Serial.println("SD card exists!");
   }
   else if (SERIALPRINT_ON) {
     Serial.println("SD card does not exist");
   }

   //Display status of data recording switch
   DataAcqSwON = digitalRead(DAcqSense);
   if (DataAcqSwON && SERIALPRINT_ON) {
     Serial.println("Data Acq switch ON");
   }
   else if (SERIALPRINT_ON) {
     Serial.println("Data Acq switch OFF");
   }
   
   //Set up MicroSD card recording file if card exists and setup has
   //not failed previously
   if (SDcardExists && DataAcqSwON && !myFileExists && !SDinitFailed) {
     setupSDFile();
   }

   //Write data to file if file exists and recording is enabled
   if (myFileExists && DataAcqSwON) {
     //writeDataToFile(&myFile, &data);  //Future work: go back to handling
                                         //file and data with pointers like before
     myFile.print(data.tm);
     myFile.print(", ");
     myFile.print(data.IRtemp1);
     myFile.print(", ");
     myFile.print(data.Dietemp1);
     myFile.print(", ");
     myFile.print(data.IRtemp2);
     myFile.print(", ");
     myFile.println(data.Dietemp2);
   }
   //Close file if file exists and recording has been turned off
   else if (myFileExists && !myFileClosed) {
     myFile.close();
     myFileClosed = true;
     if (SERIALPRINT_ON) {
        Serial.println("File closed");
     }
   }

   //Wait for next IR sensor readings
   delay(1000); 
   
}//End of void loop() ------------------------------------------------------



//----User Defined Functions----------------------------------------------------------------------

//Set up MicroSD card for writing data
void setupSDFile()
  {
    Serial.print("Initializing SD card...");
    
    //Set up chip select pin
    pinMode(chipSelect, OUTPUT);

    //Attempt to set up data file if file does not exist. If initialization
    //is unsuccessful, set Failed flag and end setup
    if (!myFileExists)
      {
         if (!SD.begin(chipSelect)) 
           {
             if (SERIALPRINT_ON) {Serial.println("SD card initialization failed!");}
             SDinitFailed = true;
             return;
           } 
         else
           {
             if (SERIALPRINT_ON) {Serial.println("Initialization successful.");}
             SDinitFailed = false;
           }  
      }
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    //Max file name length of 8 characters
    myFile = SD.open(SDFilename, FILE_WRITE);

    //If the file opened, write headers:
    if (myFile && !myFileExists) 
      {
        if (SERIALPRINT_ON) {
          Serial.println("myFile exists");
        }
        
        //Print column headers
        myFile.println("TimeMinutes,IRTemp1F,DieTemp1F,IRTemp2F,DieTemp2F");
        myFileExists = true;
      } 
    else if (!myFile)
      {
        //If the file didn't open, print error:
        if (SERIALPRINT_ON) {
          Serial.println("error opening data recording file");
        }
        
        myFileExists = false;
      }
    else if (SERIALPRINT_ON)
      {
        //If the file already exists, don't write headers and simply append data
        Serial.println("file already exists, appending data");
      }
  }
  
