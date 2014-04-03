#include <SoftwareSerial.h> 
#include <LiquidCrystal.h>
 
#define RXPIN 3
#define TXPIN 4

char command_scantag[]={0x18,0x03,0xFF};//const
int  incomingByte;
SoftwareSerial myserial(RXPIN,TXPIN) ; 
 
void setup()
{
     Serial.begin(9600);
     myserial.begin(9600);
}  
void loop()
{
   // turn on antenna
   while(Serial.available())
  {
    incomingByte = Serial.read();   
    if(incomingByte='r') 
    {
      myserial.print(command_scantag);
    } 
    
  }
  delay (1000);
   while (myserial.available())
    {
      incomingByte = myserial.read();
      Serial.println(incomingByte,HEX);
    }    
}
