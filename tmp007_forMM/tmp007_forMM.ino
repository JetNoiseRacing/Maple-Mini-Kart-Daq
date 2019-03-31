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
 
#include <Wire.h>
#include "Adafruit_TMP007_1.8.5.h"

#define BLUE_LED    12
#define GREEN_LED   13
#define RED_LED     14
#define BLUE_THRESH 70
#define RED_THRESH  80

#define SERIALPRINT_ON true

// Connect VCC to +3V (its a quieter supply than the 5V supply on an Arduino
// Gnd -> Gnd
// SCL connects to the I2C clock pin. On newer boards this is labeled with SCL
// otherwise, on the Uno, this is A5 on the Mega it is 21 and on the Leonardo/Micro digital 3
// SDA connects to the I2C data pin. On newer boards this is labeled with SDA
// otherwise, on the Uno, this is A4 on the Mega it is 20 and on the Leonardo/Micro digital 2

//Adafruit_TMP007 tmp007;
Adafruit_TMP007 tmp007(0x40);  // start with a diferent i2c address!

void setup() { 
  if (SERIALPRINT_ON) {
    Serial.begin(9600);
    while(!Serial){};  // wait on USB is up and running
    Serial.println("Adafruit TMP007 example");
  }
  
  // you can also use tmp007.begin(TMP007_CFG_1SAMPLE) or 2SAMPLE/4SAMPLE/8SAMPLE to have
  // lower precision, higher rate sampling. default is TMP007_CFG_16SAMPLE which takes
  // 4 seconds per reading (16 samples)
  uint8_t tst = 0x02;
  if (! tmp007.begin(TMP007_CFG_1SAMPLE)) {
    if (SERIALPRINT_ON) {
      Serial.println("No sensor found");
    }
    while (1);
  }
  else {
    if (SERIALPRINT_ON) {
      Serial.println("Sensor found!");
    }
  }

  pinMode(BLUE_LED,OUTPUT);
  pinMode(GREEN_LED,OUTPUT);
  pinMode(RED_LED,OUTPUT);
}

void loop() {
   float objt = tmp007.readObjTempC()*9/5+32;
   float diet = tmp007.readDieTempC()*9/5+32;

   if (SERIALPRINT_ON) {
     Serial.print("Object Temperature: "); Serial.print(objt); Serial.println("*F");
     Serial.print("Die Temperature:    "); Serial.print(diet); Serial.println("*F");
   }

   if (objt<BLUE_THRESH) {
     digitalWrite(BLUE_LED,HIGH);
     digitalWrite(GREEN_LED,LOW);
     digitalWrite(RED_LED,LOW);
   }
   else if (objt>RED_THRESH) {
     digitalWrite(BLUE_LED,LOW);
     digitalWrite(GREEN_LED,LOW);
     digitalWrite(RED_LED,HIGH);
   }
   else {
     digitalWrite(BLUE_LED,LOW);
     digitalWrite(GREEN_LED,HIGH);
     digitalWrite(RED_LED,LOW);
   }
   delay(250); // .25 seconds per reading for 1 samples per reading
   //delay(4000); // 4 seconds per reading for 16 samples per reading
}
