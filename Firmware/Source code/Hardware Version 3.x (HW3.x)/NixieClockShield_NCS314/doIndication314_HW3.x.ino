//driver for NCS314 HW3.x (registers HV57708)
//driver version 1.1
//1.1 SPI mode changed to MODE3
//0 on register's input will turn on a digit (when pol=high)

#include "doIndication314_HW3.x.h"

#define UpperDotsMask 0x80000000
#define LowerDotsMask 0x40000000

void SPISetup()
{
  SPI.begin(); //
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST /*LSBFIRST*/, SPI_MODE3)); // по датащиту получается вроде как режим 0, но мне кажется что он не подходит: выводит одновременно несколько цифр
                                                                                // режим 1 делает вид что работает выводит 3 <- эти два режима работют наиболее адекватно
                                                                                // режим 2 делает вид что работает выводит 7
                                                                                // режим 3 делает вид что работает выводит 3 <- эти два режима работют наиболее адекватно
}

void doIndication()
{
  
  static unsigned long lastTimeInterval1Started;
  if ((micros()-lastTimeInterval1Started)<fpsLimit) return;
  //if (menuPosition==TimeIndex) doDotBlink();
  lastTimeInterval1Started=micros();
    
  unsigned long Var32=0;
  unsigned long New32_L=0;
  unsigned long New32_H=0;
  
  long digits=stringToDisplay.toInt();
  
  /**********************************************************
   * Data format incomin [H1][H2}[M1][M2][S1][Y1][Y2]
   *********************************************************/
   
 //-------- REG 1 ----------------------------------------------- 
  Var32=0;
 
  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(5))<<20; // s2
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(4))<<10; //s1
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(3)); //m2
  digits=digits/10;

  if (LD) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;
  
  if (UD) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;  

  for (int i=1; i<=32; i++)
  {
   i=i+32;
   int newindex=16*(3-(ceil((float)i/4)*4-i))+ceil((float)i/4); 
   i=i-32;
   /*Serial.print("i=");
   Serial.print(i);
   Serial.print(" newindex=");
   Serial.println(newindex);*/
   if (newindex<=32) bitWrite(New32_L, newindex-1, bitRead(Var32, i-1));
    else bitWrite(New32_H, newindex-32-1, bitRead(Var32, i-1));
  }
 //-------------------------------------------------------------------------

 //-------- REG 0 ----------------------------------------------- 
  Var32=0;
  
  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(2))<<20; // m1
  digits=digits/10;

  Var32|= (unsigned long)(SymbolArray[digits%10]&doEditBlink(1))<<10; //h2
  digits=digits/10;

  Var32|= (unsigned long)SymbolArray[digits%10]&doEditBlink(0); //h1
  digits=digits/10;

  if (LD) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;
  
  if (UD) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;  

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

  digitalWrite(LEpin, HIGH); //<<-- это правильно H -> L
  digitalWrite(LEpin, LOW); // <<-- это правильно H -> L
//-------------------------------------------------------------------------
}

word doEditBlink(int pos)
{
  /*
  if (!BlinkUp) return 0;
  if (!BlinkDown) return 0;
  */

  if (!BlinkUp) return 0xFFFF;
  if (!BlinkDown) return 0xFFFF;
  //if (pos==5) return 0xFFFF; //need to be deleted for testing purpose only!
  int lowBit=blinkMask>>pos;
  lowBit=lowBit&B00000001;
  
  static unsigned long lastTimeEditBlink=millis();
  static bool blinkState=false;
  word mask=0xFFFF;
  static int tmp=0;//blinkMask;
  if ((millis()-lastTimeEditBlink)>300) 
  {
    lastTimeEditBlink=millis();
    blinkState=!blinkState;
    if (blinkState) tmp= 0;
      else tmp=blinkMask;
  }
  if (((dotPattern&~tmp)>>6)&1==1) LD=true;//digitalWrite(pinLowerDots, HIGH);
      else LD=false; //digitalWrite(pinLowerDots, LOW);
  if (((dotPattern&~tmp)>>7)&1==1) UD=true; //digitalWrite(pinUpperDots, HIGH);
      else UD=false; //digitalWrite(pinUpperDots, LOW);
      
  if ((blinkState==true) && (lowBit==1)) mask=0x3C00;//mask=B11111111;
  //Serial.print("doeditblinkMask=");
  //Serial.println(mask, BIN);
  return mask;
}

word blankDigit(int pos)
{
  int lowBit = blankMask >> pos;
  lowBit = lowBit & B00000001;
  word mask = 0;
  if (lowBit == 1) mask = 0xFFFF;
  return mask;
}
