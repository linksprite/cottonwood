/*
 
  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
*/ 
// include the library code:
#include <LiquidCrystal.h>
#define BUFFSIZ 90

//RFID parser
char buffer_RFID[BUFFSIZ];
char buffidx_RFID;
char response_str[64];

char command_scantag[]={0x31,0x03,0x01};//const

unsigned char  incomingByte;

unsigned char parsed_okay=0;

unsigned char tag_found_number;

unsigned int bytecount=0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup(){
  
  parsed_okay=0;
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Xbee UHFRFID Reader");
  
  Serial.begin(9600);
  Serial1.begin(115200);


  
}

void loop() 
{
  


  while(Serial.available())
  {
    incomingByte = Serial.read();
   
    Serial1.print(incomingByte);
  }

  while(Serial1.available())
  {
    incomingByte = Serial1.read();
    Serial.print(incomingByte);
    
  }
  

  
  

   
}