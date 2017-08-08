//const String FirmwareVersion="010210";
//Format                _X.XXX_    
//NIXIE CLOCK SHIELD NCS314 v 1.2 by GRA & AFCH (fominalec@gmail.com)
//1.021 31.01.2017
//Added: time synchronizing each 10 seconds
//Fixed: not correct time reading from RTC while start up
//1.02 17.10.2016
//Fixed: RGB color controls
//Update to Arduino IDE 1.6.12 (Time.h replaced to TimeLib.h)
//1.01
//Added RGB LEDs lock(by UP and Down Buttons)
//Added Down and Up buttons pause and resume self testing
//25.09.2016 update to HW ver 1.1
//25.05.2016
 
#include <SPI.h>
//#include <Wire.h>
//#include <ClickButton.h>
//#include <TimeLib.h>
//#include <Tone.h>
//#include <EEPROM.h>
//#include <OneWire.h>
//#include <IRremote.h>

//int RECV_PIN = 4;
//IRrecv irrecv(RECV_PIN);
//decode_results results;

const byte LEpin=10; //pin Latch Enabled data accepted while HI level
const byte DHVpin=5; // off/on MAX1771 Driver  Hight Voltage(DHV) 110-220V 


//OneWire  ds(pinTemp);

String stringToDisplay="000000";// Conten of this string will be displayed on tubes (must be 6 chars length)


//                      0      1      2      3      4      5      6      7      8       9
word SymbolArray[10]={65534, 65533, 65531, 65527, 65519, 65503, 65471, 65407, 65279, 65023};


/*******************************************************************************************************
Init Programm
*******************************************************************************************************/
void setup() 
{
  pinMode(DHVpin, OUTPUT);
  digitalWrite(DHVpin, LOW);    // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
  
  Serial.begin(115200);
  
  pinMode(LEpin, OUTPUT);

 // SPI setup
  SPI.begin(); // 
  SPI.setDataMode (SPI_MODE3); // Mode 3 SPI
  SPI.setClockDivider(SPI_CLOCK_DIV128); // SCK = 16MHz/128= 125kHz

  digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
}

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
void loop() {
  stringToDisplay="123456";
  doIndication();
  delay(3000);
  stringToDisplay="654321";
  doIndication();
  delay(3000);
}


void doIndication()
{
  digitalWrite(LEpin, LOW);    // allow data input (Transparent mode)
  unsigned long long Var64=0;
  unsigned long long tmpVar64=0;
  
  long digits=stringToDisplay.toInt();
  
  Var64=~Var64;
  tmpVar64=~tmpVar64;
  
  Var64=Var64&((SymbolArray[digits%10])<<6);
  Var64=Var64<<48;
  digits=digits/10;
  
  tmpVar64=(SymbolArray[digits%10])<<6;
  Var64|=(tmpVar64<<38);
  digits=digits/10;

  tmpVar64=(SymbolArray[digits%10])<<6;
  Var64|=(tmpVar64<<28);
  digits=digits/10;
  
  tmpVar64=(SymbolArray[digits%10])<<6;
  Var64|=(tmpVar64<<18);
  digits=digits/10;
  
  tmpVar64=(SymbolArray[digits%10])<<6;
  Var64|=(tmpVar64<<8);
  digits=digits/10;
  
  tmpVar64=(SymbolArray[digits%10])<<6>>2;
  Var64|=tmpVar64;
  
  Var64=(Var64>>4);
    
  unsigned int iTmp=0;
  
  iTmp=Var64>>56;
  SPI.transfer(iTmp);
  iTmp=Var64>>48;
  SPI.transfer(iTmp);
  iTmp=Var64>>40;
  SPI.transfer(iTmp);
  iTmp=Var64>>32;
  SPI.transfer(iTmp);
  iTmp=Var64>>24;
  SPI.transfer(iTmp);
  iTmp=Var64>>16;
  SPI.transfer(iTmp);
  iTmp=Var64>>8;
  SPI.transfer(iTmp);
  iTmp=Var64;
  SPI.transfer(iTmp);

  digitalWrite(LEpin, HIGH);    // allow data input (Transparent mode)    
  digitalWrite(LEpin, LOW);     // latching data 
}


