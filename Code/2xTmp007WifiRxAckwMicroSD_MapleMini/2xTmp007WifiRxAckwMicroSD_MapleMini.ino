/*************************************************** 
  This is to be used with 1xTmp007WifiTxAck1/2_MapleMini sketches
  for transmitting/receiving TMP007 Barometric 
  Pressure & Temp Sensor data over wifi (nRF24L01)

  This code is for the central controller, which receives IR
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

#define CE_PIN      10                          //Define CE pin for RF24 radio
#define CSN_PIN     11                          //Define CSN pin for RF24 radio

#define DAcqHi      17                          //Define pins used for data recording start/stop switch
#define DAcqSense   18                          //Hi = 3.3V source, Record when Sense = HIGH

#define chipSelect  7                           //Specify digital pin for SD card chipselect
#define ChipDet     3                           //Pin for detecting SD card

boolean SERIALPRINT_ON = false;                 //Default serial print to false, set true later if Serial exists

String SDFilename = "DATA000.txt";              //Specify desired name for SD card data file

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

File myFile;                                    //Set file object for MicroSD outfile

SPIClass SPI_2(2);                              //Set SPI #2
RF24 radio(CE_PIN,CSN_PIN);                     //Create RF24 radio object

const byte numSens = 2;                         //Number of remote sensors with which hub communcates

typedef struct                                  //Define central struct as struct of sensor structs
  {
    IRsensDEF IRsens[2]; 
  } central_data_struct;

central_data_struct data;                       //Create instance of central struct for data logging
  
const byte addresses[numSens][6] = {"00001","00002"}; //Define addresses for radio

char requestMsg[10] = "ToSenN  0";              //Compose main body of message for AckPayload message
char msgNum = '0';                              //Variable for incrementing AckPayload message number

int ackIRtempData[3] = {0,0,0};                 //Vector to contain data received from sensors

unsigned long currentMillis = 0;                //Variables for polling sensors at 1Hz
unsigned long prevMillis = 0;
unsigned long intervMillis = 1000;

bool myFileClosed = false;                      //SD card file closed
bool myFileExists = false;                      //SD card file created
bool SDinitFailed = false;                      //SD card failed to initialize
int SDcardExists = 0;                           //SD card detected
int DataAcqSwON = 0;                            //User flipped data acquisition switch

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

  pinMode(ChipDet,INPUT);                       //Set up pin for detecting SD card
  pinMode(DAcqHi,OUTPUT);                       //Set up pin for HIGH side of data acq On/Off switch
  pinMode(DAcqSense,INPUT);                     //Set pin for detecting position of data acq On/Off switch

  digitalWrite(DAcqHi,HIGH);                    //Set HIGH pin for data acq On/Off switch

} //End of void setup() ------------------------------------------------------


//----Start Loop--------------------------------------------------------------
void loop() {

  currentMillis = millis();                     //Get time

  if (currentMillis-prevMillis>=intervMillis) { //Get IR temperatures in 1-second intervals
    
    SPI.setModule(2);                           //Set SPI focus to SPI #2 for radio - Maple Mini-unique function
    
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

    SDcardExists = digitalRead(ChipDet);          //Detect whether SD card is present
    if (SDcardExists && SERIALPRINT_ON) {         //Print status of SD card if Serial open
     Serial.println("SD card exists!");
   }
   else if (SERIALPRINT_ON) {
     Serial.println("SD card does not exist");
   }

   DataAcqSwON = digitalRead(DAcqSense);          //Detect whether data acq switch is ON
   if (DataAcqSwON && SERIALPRINT_ON) {           //Print status of switch if Serial open
     Serial.println("Data Acq switch ON");
   }
   else if (SERIALPRINT_ON) {
     Serial.println("Data Acq switch OFF");
   }

   SPI.setModule(1);                              //Set SPI focus to SPI #1 for SD card - Maple Mini-unique function
   
   if (SDcardExists && DataAcqSwON &&             //Set up MicroSD card recording file if card
       !myFileExists && !SDinitFailed) {          //exists and setup has not failed previously
     setupSDFile();                               
   }

   if (myFileExists && DataAcqSwON) {
     //writeDataToFile(&myFile, &data);           //TODO: go back to handling
                                                  //file and data with pointers like before
     for (int n=0; n<numSens; n++) {
       myFile.print(data.IRsens[n].tm);
       myFile.print(", ");
       myFile.print(data.IRsens[n].count);
       myFile.print(", ");
       myFile.print(data.IRsens[n].IRtemp);
       myFile.print(", ");
       myFile.print(data.IRsens[n].Dietemp);
       myFile.print(", ");
       myFile.print(data.IRsens[n].fault);
       myFile.print(", ");
     }
     myFile.print("\n");  
   }
   else if (myFileExists && !myFileClosed) {
     myFile.close();
     myFileClosed = true;
     if (SERIALPRINT_ON) {
        Serial.println("File closed");
     }
   }
    
    msgNum+=1;                                    //Increment AckPayload message number,
    if (msgNum>'9') {                             //keeping between 1 and 9
      msgNum = '0';
    }
    requestMsg[8] = msgNum;                       //Update AckPayload message with new message number
    prevMillis = currentMillis;                   //Reset timestamp of previous transmission
  }
}

//----User Defined Functions----------------------------------------------------------------------

//Set up MicroSD card for writing data
void setupSDFile()
  {
    if (SERIALPRINT_ON){
      Serial.print("Initializing SD card...");
    }
    
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
        myFile.println("TimeSec,Count_1,IRTempF_1,DieTempF_1,Fault_1,,Count_2,IRTempF_2,DieTempF_2,Fault_2");
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
  


