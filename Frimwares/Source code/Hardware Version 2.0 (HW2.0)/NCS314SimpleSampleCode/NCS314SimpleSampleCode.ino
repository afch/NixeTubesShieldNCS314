  
//NIXIE CLOCK SHIELD NCS314 v 2.x by GRA & AFCH (fominalec@gmail.com)
//1 26.06.2018
//Simple code for HV5122PJ-G Hihg Voltage registers
 
#include <SPI.h>

const byte LEpin=10; //pin Latch Enabled data accepted while HI level

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

 // SPI setup
  SPI.begin(); // 
  SPI.setDataMode (SPI_MODE2); // Mode 2 SPI
  SPI.setClockDivider(SPI_CLOCK_DIV8); // SCK = 16MHz/8= 2MHz

}

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
void loop() {
  stringToDisplay="123456";
  Serial.println(stringToDisplay);
  doIndication();
  delay(3000);
  stringToDisplay="654321";
  Serial.println(stringToDisplay);
  doIndication();
  delay(3000);
}


void doIndication()
{
  digitalWrite(LEpin, LOW);    // allow data input (Transparent mode)
  unsigned long Var32=0;
   
  long digits=stringToDisplay.toInt();
  
  //-------- REG 1 ----------------------------------------------- 
  Var32=0;
 
  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // s2
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10])<<10; //s1
  digits=digits/10;

  Var32|=(unsigned long) (SymbolArray[digits%10]); //m2
  digits=digits/10;

  SPI.transfer(Var32>>24);
  SPI.transfer(Var32>>16);
  SPI.transfer(Var32>>8);
  SPI.transfer(Var32);

  //digitalWrite(LEpin, LOW);   
 //-------------------------------------------------------------------------

 //-------- REG 0 ----------------------------------------------- 
  Var32=0;
  
  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // m1
  digits=digits/10;

  Var32|= (unsigned long)(SymbolArray[digits%10])<<10; //h2
  digits=digits/10;

  Var32|= (unsigned long)SymbolArray[digits%10]; //h1
  digits=digits/10;

  //digitalWrite(LEpin, HIGH);
  
  SPI.transfer(Var32>>24);
  SPI.transfer(Var32>>16);
  SPI.transfer(Var32>>8);
  SPI.transfer(Var32);

  digitalWrite(LEpin, HIGH);     // latching data 
}


