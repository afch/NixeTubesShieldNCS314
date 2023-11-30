//driver for NCM107+NCT318+NCT818, NCS312, NCS314 HW2.x (registers HV5122)
//driver version 1.3
//1 on register's output will turn on a digit 

//v1.3(HV5222 MOD fix)
//v1.2(HV5222 MOD)
//v1.2 SPI setup moved to driver's file
//v1.1 Mixed up on/off for dots

#include "doIndication314_HW2.x.h"

void SPISetup()
{
  pinMode(RHV5222PIN, INPUT_PULLUP);
  HV5222=!digitalRead(RHV5222PIN);
  SPI.begin(); //

  if (HV5222)
    SPI.beginTransaction(SPISettings(2000000, LSBFIRST, SPI_MODE2));
    else SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE2));
}

void TurnOffAllTubes()
{
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);

  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);

  digitalWrite(LEpin, HIGH); //<<-- это правильно H -> L
  digitalWrite(LEpin, LOW); // <<-- это правильно H -> L
}

void doIndication()
{
  
  static unsigned long lastTimeInterval1Started;
  if ((micros()-lastTimeInterval1Started)<fpsLimit) return;
  //if (menuPosition==TimeIndex) doDotBlink();
  lastTimeInterval1Started=micros();

  if (NightMode) {TurnOffAllTubes(); return;}
    
  unsigned long Var32=0;
  
  long digits=stringToDisplay.toInt();
  //long digits=12345678;
  //Serial.print("strtoD=");
  //Serial.println(stringToDisplay);
  
  /**********************************************************
   * Подготавливаем данные по 32бита 3 раза
   * Формат данных [H1][H2}[M1][M2][S1][Y1][Y2]
   *********************************************************/
   
  digitalWrite(LEpin, LOW); 
 //-------- REG 1 ----------------------------------------------- 
  Var32=0;
 
  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(5))<<20; // s2
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(4))<<10; //s1
  digits=digits/10;

  Var32|=(unsigned long) (SymbolArray[digits%10]&doEditBlink(3)); //m2
  digits=digits/10;

  if (LD) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;
  
  if (UD) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;  

  if (HV5222) 
  {
    SPI.transfer(Var32);
    SPI.transfer(Var32>>8);
    SPI.transfer(Var32>>16);
    SPI.transfer(Var32>>24);
  } else 
  {
    SPI.transfer(Var32>>24);
    SPI.transfer(Var32>>16);
    SPI.transfer(Var32>>8);
    SPI.transfer(Var32);
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
     
  if (HV5222) 
  {
    SPI.transfer(Var32);
    SPI.transfer(Var32>>8);
    SPI.transfer(Var32>>16);
    SPI.transfer(Var32>>24);
  } else
  {
    SPI.transfer(Var32>>24);
    SPI.transfer(Var32>>16);
    SPI.transfer(Var32>>8);
    SPI.transfer(Var32);
  }

  digitalWrite(LEpin, HIGH);    
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
