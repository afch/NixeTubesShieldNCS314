//NIXIE CLOCK SHIELD NCS314 v 3.x by GRA & AFCH (fominalec@gmail.com)
//1.1 08.05.2023
//fixed: bug with SHDN pin
//1 25.04.2023
//Simple code for HV57708 High Voltage registers
 
#include <SPI.h>

const byte LEpin=10; //pin Latch Enabled data accepted while HI level
#define pinSHDN 5 // turns off tubes when LOW

String stringToDisplay="000000";// Conten of this string will be displayed on tubes (must be 6 chars length)

//                            0      1      2      3      4      5      6      7      8       9
unsigned int SymbolArray[10]={1,     2,     4,     8,     16,    32,    64,    128,   256,    512};


/*******************************************************************************************************
Init Programm
*******************************************************************************************************/
void setup() 
{
   Serial.begin(115200);
  Serial.println(F("Starting"));
  
  pinMode(LEpin, OUTPUT);
  
  pinMode(pinSHDN, OUTPUT);
  digitalWrite(pinSHDN, HIGH);  //turn on the lamps

 // SPI setup
  SPI.begin();  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
}

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
void loop() {
  stringToDisplay="123456"; //set numbers to output
  Serial.println(stringToDisplay); //output given digits to serial port (optional)
  doIndication(); //display previously set numbers on the tubes
  delay(3000); //three second delay
  stringToDisplay="654321"; //set new numbers to output (just for fun)
  Serial.println(stringToDisplay); //output given digits to serial port (optional)
  doIndication(); //display previously set numbers on the tubes
  delay(3000); //three second delay
} // returns to the first line of the loop function, which causes the digits to change from 123456 to 654321 indefinitely every three seconds.


void doIndication()
{
  unsigned long Var32=0;
  unsigned long New32_L=0;
  unsigned long New32_H=0;
  
  long digits=stringToDisplay.toInt();
  
  /**********************************************************
   * Data format incomin [H1][H2}[M1][M2][S1][Y1][Y2]
   *********************************************************/
   
 //-------- REG 1 ----------------------------------------------- 
  Var32=0;
 
  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // s2
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10])<<10; //s1
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10]); //m2
  digits=digits/10;

  for (int i=1; i<=32; i++)
  {
   i=i+32;
   int newindex=16*(3-(ceil((float)i/4)*4-i))+ceil((float)i/4); 
   i=i-32;
   if (newindex<=32) bitWrite(New32_L, newindex-1, bitRead(Var32, i-1));
    else bitWrite(New32_H, newindex-32-1, bitRead(Var32, i-1));
  }
 //-------------------------------------------------------------------------

 //-------- REG 0 ----------------------------------------------- 
  Var32=0;
  
  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // m1
  digits=digits/10;

  Var32|= (unsigned long)(SymbolArray[digits%10])<<10; //h2
  digits=digits/10;

  Var32|= (unsigned long)SymbolArray[digits%10]; //h1
  digits=digits/10;

  for (int i=1; i<=32; i++)
  {
   int newindex=16*(3-(ceil((float)i/4)*4-i))+ceil((float)i/4); 
   if (newindex<=32) bitWrite(New32_L, newindex-1, bitRead(Var32, i-1));
    else bitWrite(New32_H, newindex-32-1, bitRead(Var32, i-1));
  }

  SPI.transfer((New32_H)>>24);
  SPI.transfer((New32_H)>>16);
  SPI.transfer((New32_H)>>8);
  SPI.transfer(New32_H);
  
  SPI.transfer((New32_L)>>24);
  SPI.transfer((New32_L)>>16);
  SPI.transfer((New32_L)>>8);
  SPI.transfer(New32_L);

  digitalWrite(LEpin, HIGH); 
  digitalWrite(LEpin, LOW); 
//-------------------------------------------------------------------------
}
