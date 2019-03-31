/*************************************************** 
  This is an example for the TMP007 Barometric Pressure & Temp Sensor

  Designed specifically to work with the Adafruit TMP007 Breakout 
  ----> https://www.adafruit.com/products/2023
  
  These displays use I2C to communicate, 2 pins are required to  
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

//Specify desired name for SD card data file
String SDFilename = "MMTMP15.txt";
 
#include <Wire.h>
#include "Adafruit_TMP007_1.8.5.h"
#include <SD.h>

#define BLUE_LED    12
#define GREEN_LED   13
#define RED_LED     14
#define BLUE_THRESH 70
#define RED_THRESH  80

#define SS          7
#define SCK         6
#define MISO        5
#define MOSI        4
#define ChipDet     3

#define DAcqHi      17
#define DAcqSense   18

boolean SERIALPRINT_ON = false;

// Connect VCC to +3V (its a quieter supply than the 5V supply on an Arduino
// Gnd -> Gnd
// SCL connects to the I2C clock pin. On newer boards this is labeled with SCL
// otherwise, on the Uno, this is A5 on the Mega it is 21 and on the Leonardo/Micro digital 3
// SDA connects to the I2C data pin. On newer boards this is labeled with SDA
// otherwise, on the Uno, this is A4 on the Mega it is 20 and on the Leonardo/Micro digital 2

//Set up TMP007 communication objects
Adafruit_TMP007 tmp007_1(0x40);  // Sensor #1
Adafruit_TMP007 tmp007_2(0x42);  // Sensor #2

//----MicroSD File Setup----------------------------------------------------------------

//Set file handle for MicroSD outfile
File myFile;

//Specify digital pin for chipselect
const int chipSelect = 7;

bool myFileClosed = false;
bool myFileExists = false;
bool SDinitFailed = false;
int SDcardExists = 0;
int DataAcqSwON = 0;

//----Data Struct Setup----------------------------------------------------------------
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
  
  if (Serial3) {
    Serial.begin(9600);
    SERIALPRINT_ON = true;
    while(!Serial){};  // Wait until Serial is up and running
    Serial.println("Adafruit 2xTMP007 example");
  }
  
  // you can also use tmp007.begin(TMP007_CFG_1SAMPLE) or 2SAMPLE/4SAMPLE/8SAMPLE to have
  // lower precision, higher rate sampling. default is TMP007_CFG_16SAMPLE which takes
  // 4 seconds per reading (16 samples)

  //Setup IR sensor #1
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

  //Setup IR sensor #2
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
  
  pinMode(BLUE_LED,OUTPUT);
  pinMode(GREEN_LED,OUTPUT);
  pinMode(RED_LED,OUTPUT);

  pinMode(ChipDet,INPUT);
  pinMode(DAcqHi,OUTPUT);
  pinMode(DAcqSense,INPUT);

  digitalWrite(DAcqHi,HIGH);

}


//----Start Loop--------------------------------------------------------------

void loop() {

   data.tm = millis();
   
   data.IRtemp1 = int(tmp007_1.readObjTempC())*9/5+32;
   data.Dietemp1 = int(tmp007_1.readDieTempC())*9/5+32;
   data.IRtemp2 = int(tmp007_2.readObjTempC())*9/5+32;
   data.Dietemp2 = int(tmp007_2.readDieTempC())*9/5+32;

   if (SERIALPRINT_ON) {
     Serial.print("Object #1 Temperature: "); Serial.print(data.IRtemp1); Serial.println("*F");
     Serial.print("Die #1 Temperature:    "); Serial.print(data.Dietemp1); Serial.println("*F");
     Serial.print("Object #2 Temperature: "); Serial.print(data.IRtemp2); Serial.println("*F");
     Serial.print("Die #2 Temperature:    "); Serial.print(data.Dietemp2); Serial.println("*F");
   }

//   if (data.IRtemp1<BLUE_THRESH) {
//     digitalWrite(BLUE_LED,HIGH);
//     digitalWrite(GREEN_LED,LOW);
//     digitalWrite(RED_LED,LOW);
//   }
//   else if (data.IRtemp1>RED_THRESH) {
//     digitalWrite(BLUE_LED,LOW);
//     digitalWrite(GREEN_LED,LOW);
//     digitalWrite(RED_LED,HIGH);
//   }
//   else {
//     digitalWrite(BLUE_LED,LOW);
//     digitalWrite(GREEN_LED,HIGH);
//     digitalWrite(RED_LED,LOW);
//   }

   SDcardExists = digitalRead(ChipDet);
   if (SDcardExists && SERIALPRINT_ON) {
     Serial.println("SD card exists!");
   }
   else if (SERIALPRINT_ON) {
     Serial.println("SD card does not exist");
   }
   
   DataAcqSwON = digitalRead(DAcqSense);
   if (DataAcqSwON && SERIALPRINT_ON) {
     Serial.println("Data Acq switch ON");
   }
   else if (SERIALPRINT_ON) {
     Serial.println("Data Acq switch OFF");
   }
   
   
   if (SDcardExists && DataAcqSwON && !myFileExists && !SDinitFailed) {
     setupSDFile();
   }

   if (myFileExists && DataAcqSwON) {
     //writeDataToFile(&myFile, &data);
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
   else if (myFileExists && !myFileClosed) {
     myFile.close();
     myFileClosed = true;
     if (SERIALPRINT_ON) {
        Serial.println("File closed");
     }
   }
   
   delay(1000); 
   //delay(4000); // 4 seconds per reading for 16 samples per reading
}




//----User Defined Functions----------------------------------------------------------------------


void setupSDFile()
  {
    Serial.print("Initializing SD card...");
    
    // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
    // Note that even if it's not used as the CS pin, the hardware SS pin 
    // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
    // or the SD library functions will not work. 
    pinMode(chipSelect, OUTPUT);

    if (!myFileExists)
      {
        if (SERIALPRINT_ON) {
          if (!SD.begin(chipSelect)) 
            {
              Serial.println("SD card initialization failed!");
              SDinitFailed = true;
              return;
            } 
          else
            {
              Serial.println("Initialization successful.");
              SDinitFailed = false;
            }
        }
      }
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    //Max file name length of 8 characters
    myFile = SD.open(SDFilename, FILE_WRITE);


   
    // if the file opened okay, write to it:
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
        // if the file didn't open, print an error:
        if (SERIALPRINT_ON) {
          Serial.println("error opening data recording file");
        }
        
        myFileExists = false;
      }
    else if (SERIALPRINT_ON)
      {
        Serial.println("file already exists, appending data");
      }
  }
  

