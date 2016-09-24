// Wire Master Reader
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Reads data from an I2C/TWI slave device
// Refer to the "Wire Slave Sender" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <Wire.h>

union byte_float_union{
  char bVal[4];
  float fVal;
} wheelspeed; 

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  Serial.println("Starting!");
}

void loop() {
  //Serial.println("Here1!");
  for(int i =0; i < 4; i++)
  {
    Wire.requestFrom(0x04, 1);    // request 6 bytes from slave device #8
  
  //Serial.println("Here2!");
while   (Wire.available()) { // slave may send less than requested
    //Serial.println("Here3!");
     wheelspeed.bVal[i] = Wire.read(); // receive a byte as character
     //Serial.println(wheelspeed.bVal[i]);
   }
  }
  //Serial.println("Here4!");
      Serial.println(wheelspeed.fVal);         // print the character

  delay(500);
}
