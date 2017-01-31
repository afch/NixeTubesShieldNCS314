const String FirmwareVersion="010210";
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
#include <Wire.h>
#include <ClickButton.h>
#include <TimeLib.h>
#include <Tone.h>
#include <EEPROM.h>
//#include <OneWire.h>
//#include <IRremote.h>

//int RECV_PIN = 4;
//IRrecv irrecv(RECV_PIN);
//decode_results results;

const byte LEpin=10; //pin Latch Enabled data accepted while HI level
const byte DHVpin=5; // off/on MAX1771 Driver  Hight Voltage(DHV) 110-220V 
const byte RedLedPin=9; //MCU WDM output for red LEDs 9-g
const byte GreenLedPin=6; //MCU WDM output for green LEDs 6-b
const byte BlueLedPin=3; //MCU WDM output for blue LEDs 3-r
const byte pinSet=A0;
const byte pinUp=A2;
const byte pinDown=A1;
const byte pinBuzzer=2;
const byte pinUpperDots=12; //HIGH value light a dots
const byte pinLowerDots=8;  //HIGH value light a dots
const word fpsLimit=16666; // 1/60*1.000.000 //limit maximum refresh rate on 60 fps
const byte pinTemp=7;
bool RTC_present;

//OneWire  ds(pinTemp);

String stringToDisplay="000000";// Conten of this string will be displayed on tubes (must be 6 chars length)
int menuPosition=0; // 0 - time
                    // 1 - date
                    // 2 - alarm
                    // 3 - 12/24 hours mode
                    
byte blinkMask=B00000000; //bit mask for blinkin digits (1 - blink, 0 - constant light)

//                      0      1      2      3      4      5      6      7      8       9
word SymbolArray[10]={65534, 65533, 65531, 65527, 65519, 65503, 65471, 65407, 65279, 65023};

byte dotPattern=B00000000; //bit mask for separeting dots 
                          //B00000000 - turn off up and down dots 
                          //B1100000 - turn off all dots

#define DS1307_ADDRESS 0x68
byte zero = 0x00; //workaround for issue #527
int RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year, RTC_day_of_week;

//--    ------------0--------1--------2-------3--------4--------5--------6--------7--------8--------9--------10-------11-------12-------13-------14
//         names:  Time,   Date,   Alarm,   12/24    hours,   mintues, seconds,  day,    month,   year,    hour,   minute,   second alarm01  hour_format 
//                  1        1        1       1        1        1        1        1        1        1        1        1        1        1        1
int parent[15]={    0,       0,       0,      0,       1,       1,       1,       2,       2,       2,       3,       3,       3,       3,       4};
int firstChild[15]={4,       7,       10,     14,      0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0};
int lastChild[15]={ 6,       9,       13,     14,      0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0};
int value[15]={     0,       0,       0,      0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,      24};
int maxValue[15]={  0,       0,       0,      0,      23,      59,      59,      31,      12,      99,      23,      59,      59,       1,      24};
int minValue[15]={  0,       0,       0,      12,     00,      00,      00,       1,       1,      00,      00,      00,      00,       0,      12};
byte blinkPattern[15]={
                    B00000000, 
                             B00000000, 
                                      B00000000, 
                                              B00000000, 
                                                      B00000011, 
                                                                B00001100, 
                                                                          B00110000, 
                                                                                  B00000011, 
                                                                                           B00001100, 
                                                                                                    B00110000, 
                                                                                                             B00000011, 
                                                                                                                      B00001100, 
                                                                                                                                B00110000,
                                                                                                                                        B11000000,  
                                                                                                                                            B00001100};
#define TimeIndex        0 
#define DateIndex        1 
#define AlarmIndex       2 
#define hModeIndex       3 
#define TimeHoursIndex   4 
#define TimeMintuesIndex 5
#define TimeSecondsIndex 6
#define DateDayIndex     7
#define DateMonthIndex   8
#define DateYearIndex    9 
#define AlarmHourIndex   10
#define AlarmMinuteIndex 11
#define AlarmSecondIndex 12
#define Alarm01          13
#define hModeValueIndex  14

bool editMode=false;

long downTime=0;
long upTime=0;
const long settingDelay=150;
bool BlinkUp=false;
bool BlinkDown=false;
unsigned long enteringEditModeTime=0;
bool RGBLedsOn=true;
byte RGBLEDsEEPROMAddress=0; 
byte HourFormatEEPROMAddress=1;
byte AlarmTimeEEPROMAddress=2;//3,4,5
byte AlarmArmedEEPROMAddress=6;
byte LEDsLockEEPROMAddress=7;   
byte LEDsRedValueEEPROMAddress=8; 
byte LEDsGreenValueEEPROMAddress=9; 
byte LEDsBlueValueEEPROMAddress=10;   

//buttons pins declarations
ClickButton setButton(pinSet, LOW, CLICKBTN_PULLUP);
ClickButton upButton(pinUp, LOW, CLICKBTN_PULLUP);
ClickButton downButton(pinDown, LOW, CLICKBTN_PULLUP);
///////////////////

Tone tone1;
#define isdigit(n) (n >= '0' && n <= '9')
//char *song = "MissionImp:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,a#,g,2d,32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d";
char *song = "PinkPanther:d=4,o=5,b=160:8d#,8e,2p,8f#,8g,2p,8d#,8e,16p,8f#,8g,16p,8c6,8b,16p,8d#,8e,16p,8b,2a#,2p,16a,16g,16e,16d,2e";
//char *song="VanessaMae:d=4,o=6,b=70:32c7,32b,16c7,32g,32p,32g,32p,32d#,32p,32d#,32p,32c,32p,32c,32p,32c7,32b,16c7,32g#,32p,32g#,32p,32f,32p,16f,32c,32p,32c,32p,32c7,32b,16c7,32g,32p,32g,32p,32d#,32p,32d#,32p,32c,32p,32c,32p,32g,32f,32d#,32d,32c,32d,32d#,32c,32d#,32f,16g,8p,16d7,32c7,32d7,32a#,32d7,32a,32d7,32g,32d7,32d7,32p,32d7,32p,32d7,32p,16d7,32c7,32d7,32a#,32d7,32a,32d7,32g,32d7,32d7,32p,32d7,32p,32d7,32p,32g,32f,32d#,32d,32c,32d,32d#,32c,32d#,32f,16c";
//char *song="DasBoot:d=4,o=5,b=100:d#.4,8d4,8c4,8d4,8d#4,8g4,a#.4,8a4,8g4,8a4,8a#4,8d,2f.,p,f.4,8e4,8d4,8e4,8f4,8a4,c.,8b4,8a4,8b4,8c,8e,2g.,2p";
//char *song="Scatman:d=4,o=5,b=200:8b,16b,32p,8b,16b,32p,8b,2d6,16p,16c#.6,16p.,8d6,16p,16c#6,8b,16p,8f#,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8f#,2p,32p,2d6,16p,16c#6,8p,16d.6,16p.,16c#6,16a.,16p.,8e,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8b,16b,32p,8b,16b,32p,8b,2d6,16p,16c#.6,16p.,8d6,16p,16c#6,8b,16p,8f#,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8f#,2p,32p,2d6,16p,16c#6,8p,16d.6,16p.,16c#6,16a.,16p.,8e,2p.,16c#6,8p,16d.6,16p.,16c#6,16a,8p,8e,2p,32p,16f#.6,16p.,16b.,16p.";
//char *song="Popcorn:d=4,o=5,b=160:8c6,8a#,8c6,8g,8d#,8g,c,8c6,8a#,8c6,8g,8d#,8g,c,8c6,8d6,8d#6,16c6,8d#6,16c6,8d#6,8d6,16a#,8d6,16a#,8d6,8c6,8a#,8g,8a#,c6";
//char *song="WeWishYou:d=4,o=5,b=200:d,g,8g,8a,8g,8f#,e,e,e,a,8a,8b,8a,8g,f#,d,d,b,8b,8c6,8b,8a,g,e,d,e,a,f#,2g,d,g,8g,8a,8g,8f#,e,e,e,a,8a,8b,8a,8g,f#,d,d,b,8b,8c6,8b,8a,g,e,d,e,a,f#,1g,d,g,g,g,2f#,f#,g,f#,e,2d,a,b,8a,8a,8g,8g,d6,d,d,e,a,f#,2g";
#define OCTAVE_OFFSET 0
char *p;

int notes[] = { 0,
NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7
};

int fireforks[]={0,0,1,//1
                -1,0,0,//2
                 0,1,0,//3
                 0,0,-1,//4
                 1,0,0,//5
                 0,-1,0}; //array with RGB rules (0 - do nothing, -1 - decrese, +1 - increse

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w=1);

int functionDownButton=0;
int functionUpButton=0;
bool LEDsLock=false;

/*******************************************************************************************************
Init Programm
*******************************************************************************************************/
void setup() 
{
  digitalWrite(DHVpin, LOW);    // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
  //irrecv.enableIRIn(); // Start the receiver  
  Wire.begin();
  //setRTCDateTime(23,40,00,25,7,15,1);
  
  Serial.begin(115200);
  //Serial1.begin(9600);
  
    if (EEPROM.read(HourFormatEEPROMAddress)!=12) value[hModeValueIndex]=24; else value[hModeValueIndex]=12;
    if (EEPROM.read(RGBLEDsEEPROMAddress)!=0) RGBLedsOn=true; else RGBLedsOn=false;
    if (EEPROM.read(AlarmTimeEEPROMAddress)==255) value[AlarmHourIndex]=0; else value[AlarmHourIndex]=EEPROM.read(AlarmTimeEEPROMAddress);
    if (EEPROM.read(AlarmTimeEEPROMAddress+1)==255) value[AlarmMinuteIndex]=0; else value[AlarmMinuteIndex]=EEPROM.read(AlarmTimeEEPROMAddress+1);
    if (EEPROM.read(AlarmTimeEEPROMAddress+2)==255) value[AlarmSecondIndex]=0; else value[AlarmSecondIndex]=EEPROM.read(AlarmTimeEEPROMAddress+2);
    if (EEPROM.read(AlarmArmedEEPROMAddress)==255) value[Alarm01]=0; else value[Alarm01]=EEPROM.read(AlarmArmedEEPROMAddress);
    if (EEPROM.read(LEDsLockEEPROMAddress)==255) LEDsLock=false; else LEDsLock=EEPROM.read(LEDsLockEEPROMAddress);
    Serial.print("led lock=");
    Serial.println(LEDsLock);
    
  pinMode(RedLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(BlueLedPin, OUTPUT);
    
  tone1.begin(pinBuzzer);
  song=parseSong(song);
  
  pinMode(LEpin, OUTPUT);
  pinMode(DHVpin, OUTPUT);
  
 // SPI setup

  SPI.begin(); // 
  SPI.setDataMode (SPI_MODE3); // Mode 3 SPI
  SPI.setClockDivider(SPI_CLOCK_DIV128); // SCK = 16MHz/128= 125kHz
 
  //buttons pins inits
  pinMode(pinSet,  INPUT_PULLUP);
  pinMode(pinUp,  INPUT_PULLUP);
  pinMode(pinDown,  INPUT_PULLUP);
  ////////////////////////////
  pinMode(pinBuzzer, OUTPUT);
  
  //buttons objects inits
  setButton.debounceTime   = 20;   // Debounce timer in ms
  setButton.multiclickTime = 30;  // Time limit for multi clicks
  setButton.longClickTime  = 2000; // time until "held-down clicks" register
  
  upButton.debounceTime   = 20;   // Debounce timer in ms
  upButton.multiclickTime = 30;  // Time limit for multi clicks
  upButton.longClickTime  = 2000; // time until "held-down clicks" register

  downButton.debounceTime   = 20;   // Debounce timer in ms
  downButton.multiclickTime = 30;  // Time limit for multi clicks
  downButton.longClickTime  = 2000; // time until "held-down clicks" register
  
  //
  //digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  doTest();
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  if (LEDsLock==1)
    {
      setLEDsFromEEPROM();
    }
  getRTCTime();
  byte prevSeconds=RTC_seconds;
  unsigned long RTC_ReadingStartTime=millis();
  RTC_present=true;
  while(prevSeconds==RTC_seconds)
  {
    getRTCTime();
    //Serial.println(RTC_seconds);
    if ((millis()-RTC_ReadingStartTime)>3000)
    {
      Serial.println(F("Warning! RTC DON'T RESPOND!"));
      RTC_present=false;
      break;
    }
  }
  setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
  digitalWrite(DHVpin, LOW); // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
  //setRTCDateTime(RTC_hours,RTC_minutes,RTC_seconds,RTC_day,RTC_month,RTC_year,1); //записываем только что считанное время в RTC чтобы запустить новую микросхему
  digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
  //p=song;
}

void rotateLeft(uint8_t &bits)
{
   uint8_t high_bit = bits & (1 << 7) ? 1 : 0;
  bits = (bits << 1) | high_bit;
}

int rotator=0; //index in array with RGB "rules" (increse by one on each 255 cycles)
int cycle=0; //cycles counter
int RedLight=255;
int GreenLight=0;
int BlueLight=0;
unsigned long prevTime=0; // time of lase tube was lit
unsigned long prevTime4FireWorks=0;  //time of last RGB changed
//int minuteL=0; //младшая цифра минут

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
void loop() {

 if (((millis()%10000)==0)&&(RTC_present)) //synchronize with RTC every 10 seconds
 {
  getRTCTime();
  setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
  Serial.println("sync");
 }
  
  p=playmusic(p);
  
  if ((millis()-prevTime4FireWorks)>5)
  {
    rotateFireWorks(); //change color (by 1 step)
    prevTime4FireWorks=millis();
  }
    
  doIndication();
  
  setButton.Update();
  upButton.Update();
  downButton.Update();
  if (editMode==false) 
    {
      blinkMask=B00000000;
      
    } else if ((millis()-enteringEditModeTime)>60000) 
    {
      editMode=false;
      menuPosition=firstChild[menuPosition];
      blinkMask=blinkPattern[menuPosition];
    }
  if (setButton.clicks>0) //short click
    {
      p=0; //shut off music )))
      tone1.play(1000,100);
      enteringEditModeTime=millis();
      menuPosition=menuPosition+1;
      if (menuPosition==hModeIndex+1) menuPosition=TimeIndex;
      Serial.print(F("menuPosition="));
      Serial.println(menuPosition);
      Serial.print(F("value="));
      Serial.println(value[menuPosition]);
      
      blinkMask=blinkPattern[menuPosition];
      if ((parent[menuPosition-1]!=0) and (lastChild[parent[menuPosition-1]-1]==(menuPosition-1))) 
      {
        if ((parent[menuPosition-1]-1==1) && (!isValidDate())) 
          {
            menuPosition=DateDayIndex;
            return;
          }
        editMode=false;
        menuPosition=parent[menuPosition-1]-1;
        if (menuPosition==TimeIndex) setTime(value[TimeHoursIndex], value[TimeMintuesIndex], value[TimeSecondsIndex], day(), month(), year());
        if (menuPosition==DateIndex) setTime(hour(), minute(), second(),value[DateDayIndex], value[DateMonthIndex], 2000+value[DateYearIndex]);
        if (menuPosition==AlarmIndex) {EEPROM.write(AlarmTimeEEPROMAddress,value[AlarmHourIndex]); EEPROM.write(AlarmTimeEEPROMAddress+1,value[AlarmMinuteIndex]); EEPROM.write(AlarmTimeEEPROMAddress+2,value[AlarmSecondIndex]); EEPROM.write(AlarmArmedEEPROMAddress, value[Alarm01]);};
        if (menuPosition==hModeIndex) EEPROM.write(HourFormatEEPROMAddress, value[hModeValueIndex]);
        digitalWrite(DHVpin, LOW); // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
        setRTCDateTime(hour(),minute(),second(),day(),month(),year()%1000,1);
        digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
      }
      value[menuPosition]=extractDigits(blinkMask);
    }
  if (setButton.clicks<0) //long click
    {
      tone1.play(1000,100);
      if (!editMode) 
      {
        enteringEditModeTime=millis();
        if (menuPosition==TimeIndex) stringToDisplay=PreZero(hour())+PreZero(minute())+PreZero(second()); //temporary enabled 24 hour format while settings
      }
      menuPosition=firstChild[menuPosition];
      if (menuPosition==AlarmHourIndex) {value[Alarm01]=1; /*digitalWrite(pinUpperDots, HIGH);*/dotPattern=B10000000;}
      editMode=!editMode;
      blinkMask=blinkPattern[menuPosition];
      value[menuPosition]=extractDigits(blinkMask);
    }
   
  if (upButton.clicks != 0) functionUpButton = upButton.clicks;  
   
  if (upButton.clicks>0) 
    {
      p=0; //shut off music )))
      tone1.play(1000,100);
      incrementValue();
      if (!editMode) 
        {
          LEDsLock=false;
          EEPROM.write(LEDsLockEEPROMAddress, 0);
        }
    }
  
  if (functionUpButton == -1 && upButton.depressed == true)   
  {
   BlinkUp=false;
   if (editMode==true) 
   {
     if ( (millis() - upTime) > settingDelay)
    {
     upTime = millis();// + settingDelay;
      incrementValue();
    }
   }
  } else BlinkUp=true;
  
  if (downButton.clicks != 0) functionDownButton = downButton.clicks;
  
  if (downButton.clicks>0) 
    {
      p=0; //shut off music )))
      tone1.play(1000,100);
      dicrementValue();
      if (!editMode) 
      {
        LEDsLock=true;
        EEPROM.write(LEDsLockEEPROMAddress, 1);
        EEPROM.write(LEDsRedValueEEPROMAddress, RedLight);
        EEPROM.write(LEDsGreenValueEEPROMAddress, GreenLight);
        EEPROM.write(LEDsBlueValueEEPROMAddress, BlueLight);
      }
    }
  
  if (functionDownButton == -1 && downButton.depressed == true)   
  {
   BlinkDown=false;
   if (editMode==true) 
   {
     if ( (millis() - downTime) > settingDelay)
    {
     downTime = millis();// + settingDelay;
      dicrementValue();
    }
   }
  } else BlinkDown=true;
  
  if (!editMode)
  {
    if (upButton.clicks<0)
      {
        tone1.play(1000,100);
        RGBLedsOn=true;
        EEPROM.write(RGBLEDsEEPROMAddress,1);
        Serial.println("RGB=on");
        setLEDsFromEEPROM();
      }
    if (downButton.clicks<0)
    {
      tone1.play(1000,100);
      RGBLedsOn=false;
      EEPROM.write(RGBLEDsEEPROMAddress,0);
      Serial.println("RGB=off");
    }
  }
  
    
  static bool updateDateTime=false;
  switch (menuPosition)
  {
    case TimeIndex: //time mode
       stringToDisplay=updateDisplayString();
       doDotBlink();
       checkAlarmTime();
       break;
    case DateIndex: //date mode
      stringToDisplay=PreZero(day())+PreZero(month())+PreZero(year()%1000);
      dotPattern=B01000000;//turn on lower dots
      /*digitalWrite(pinUpperDots, LOW);
      digitalWrite(pinLowerDots, HIGH);*/
      checkAlarmTime();
      break;
    case AlarmIndex: //alarm mode
      stringToDisplay=PreZero(value[AlarmHourIndex])+PreZero(value[AlarmMinuteIndex])+PreZero(value[AlarmSecondIndex]);
     if (value[Alarm01]==1) /*digitalWrite(pinUpperDots, HIGH);*/ dotPattern=B10000000; //turn on upper dots
           else 
           {
             /*digitalWrite(pinUpperDots, LOW);
             digitalWrite(pinLowerDots, LOW);*/
             dotPattern=B00000000; //turn off upper dots
           }
      checkAlarmTime();
      break;
    case hModeIndex: //12/24 hours mode
      stringToDisplay="00"+String(value[hModeValueIndex])+"00";
      dotPattern=B00000000;//turn off all dots
      /*digitalWrite(pinUpperDots, LOW);
      digitalWrite(pinLowerDots, LOW);*/
      checkAlarmTime();
      break;
  }
}

String PreZero(int digit)
{
  if (digit<10) return String("0")+String(digit);
    else return String(digit);
}

void rotateFireWorks()
{
  if (!RGBLedsOn)
  {
    analogWrite(RedLedPin,0 );
    analogWrite(GreenLedPin,0);
    analogWrite(BlueLedPin,0); 
    return;
  }
  if (LEDsLock) return;
  RedLight=RedLight+fireforks[rotator*3];
  GreenLight=GreenLight+fireforks[rotator*3+1];
  BlueLight=BlueLight+fireforks[rotator*3+2];
  analogWrite(RedLedPin,RedLight );
  analogWrite(GreenLedPin,GreenLight);
  analogWrite(BlueLedPin,BlueLight);  
  cycle=cycle+1;
  if (cycle==255)
  {  
    rotator=rotator+1;
    cycle=0;
  }
  if (rotator>5) rotator=0;
}


void doIndication()
{
  
  static unsigned long lastTimeInterval1Started;
  if ((micros()-lastTimeInterval1Started)<fpsLimit) return;
  //if (menuPosition==TimeIndex) doDotBlink();
  lastTimeInterval1Started=micros();
    
  digitalWrite(LEpin, LOW);    // allow data input (Transparent mode)
   
  /* 
    SPI.transfer((b|dotPattern) & doEditBlink()); // anode
    SPI.transfer(highBytesArray[stringToDisplay.substring(curTube,curTube+1).toInt()]);
    SPI.transfer(lowBytesArray[stringToDisplay.substring(curTube,curTube+1).toInt()]);
  */
  //word SymbolArray2[10]={65534, 65023, 65279, 65407, 65471, 65503, 65519, 65527, 65531, 65533}; // used only for shiled with HW ver 1.0 //it has hardware bug in wiring
  
  unsigned long long Var64=0;
  unsigned long long tmpVar64=0;
  
  long digits=stringToDisplay.toInt();
  
  Var64=~Var64;
  tmpVar64=~tmpVar64;
  
  Var64=Var64&((SymbolArray[digits%10]|doEditBlink(5))<<6);
  Var64=Var64<<48;
  digits=digits/10;
  
  tmpVar64=(SymbolArray[digits%10]|doEditBlink(4))<<6;
  Var64|=(tmpVar64<<38);
  digits=digits/10;

  tmpVar64=(SymbolArray[digits%10]|doEditBlink(3))<<6;
  Var64|=(tmpVar64<<28);
  digits=digits/10;
  
  tmpVar64=(SymbolArray[digits%10]|doEditBlink(2))<<6;
  Var64|=(tmpVar64<<18);
  digits=digits/10;
  
  tmpVar64=(SymbolArray[digits%10]|doEditBlink(1))<<6;
  Var64|=(tmpVar64<<8);
  digits=digits/10;
  
  tmpVar64=(SymbolArray[digits%10]|doEditBlink(0))<<6>>2;
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

byte CheckButtonsState()
{
  static boolean buttonsWasChecked;
  static unsigned long startBuzzTime;
  static unsigned long lastTimeButtonsPressed;
  if ((digitalRead(pinSet)==0)||(digitalRead(pinUp)==0)||(digitalRead(pinDown)==0))
  {
    if (buttonsWasChecked==false) startBuzzTime=millis();
    buttonsWasChecked=true;
  } else buttonsWasChecked=false;
  if (millis()-startBuzzTime<30) 
    {
      digitalWrite(pinBuzzer, HIGH);
    } else
    {
      digitalWrite(pinBuzzer, LOW);
    }
}

String updateDisplayString()
{
  static  unsigned long lastTimeStringWasUpdated;
  if ((millis()-lastTimeStringWasUpdated)>1000)
  {
    //Serial.println("doDotBlink");
    //doDotBlink();
    lastTimeStringWasUpdated=millis();
    if (value[hModeValueIndex]==24) return PreZero(hour())+PreZero(minute())+PreZero(second());
      else return PreZero(hourFormat12())+PreZero(minute())+PreZero(second());
    
  }
  return stringToDisplay;
}

void doTest()
{
  Serial.print(F("Firmware version: "));
  Serial.println(FirmwareVersion.substring(1,2)+"."+FirmwareVersion.substring(2,5));
  Serial.println(F("Start Test"));
  int adc=analogRead(A3);
  float Uinput=4.6*(5.0*adc)/1024.0+0.7;
  Serial.print(F("U input="));
  Serial.print(Uinput);
  
  p=song;
  parseSong(p);
   
  analogWrite(RedLedPin,255);
  delay(1000);
  analogWrite(RedLedPin,0);
  analogWrite(GreenLedPin,255);
  delay(1000);
  analogWrite(GreenLedPin,0);
  analogWrite(BlueLedPin,255);
  delay(1000); 
  //while(1);
  
  String testStringArray[12]={"000000","111111","222222","333333","444444","555555","666666","777777","888888","999999","",""};

  if (Uinput<10) testStringArray[10]="000"+String(int(Uinput*100)); else testStringArray[10]="00"+String(int(Uinput*100));
  testStringArray[11]=FirmwareVersion;

  int dlay=500;
  bool test=1;
  byte strIndex=-1;
  unsigned long startOfTest=millis();
  digitalWrite(DHVpin, HIGH);
  bool digitsLock=false;
  while (test)
  {
    if (digitalRead(pinDown)==0) digitsLock=true;
    if (digitalRead(pinUp)==0) digitsLock=false;
  for (int i=0; i<12; i++)
  {
   if ((millis()-startOfTest)>dlay) 
   {
     startOfTest=millis();
     if (!digitsLock) strIndex=strIndex+1;
     if (strIndex==10) dlay=3000;
     if (strIndex==12) test=0;
     
     stringToDisplay=testStringArray[strIndex];
     long digits=stringToDisplay.toInt();
   
    unsigned long long Var64=0;
    unsigned long long tmpVar64=0;
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
  
    digitalWrite(LEpin, HIGH);    // allow data input (Transparent mode)
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
      
    digitalWrite(LEpin, LOW);     // latching data 
   }
  }
   delayMicroseconds(2000);
  }; 

  Serial.println(F("Stop Test"));
  
}

void doDotBlink()
{
  static unsigned long lastTimeBlink=millis();
  static bool dotState=0;
  if ((millis()-lastTimeBlink)>1000) 
  {
    lastTimeBlink=millis();
    dotState=!dotState;
    if (dotState) 
      {
        dotPattern=B11000000;
        /*digitalWrite(pinUpperDots, HIGH);
        digitalWrite(pinLowerDots, HIGH);*/
      }
      else 
      {
        dotPattern=B00000000;
        /*digitalWrite(pinUpperDots, LOW); 
        digitalWrite(pinLowerDots, LOW);*/
      }
  }
}

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w)
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); //stop Oscillator

  Wire.write(decToBcd(s));
  Wire.write(decToBcd(m));
  Wire.write(decToBcd(h));
  Wire.write(decToBcd(w));
  Wire.write(decToBcd(d));
  Wire.write(decToBcd(mon));
  Wire.write(decToBcd(y));

  Wire.write(zero); //start 

  Wire.endTransmission();

}

byte decToBcd(byte val){
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)  {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

void getRTCTime()
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  RTC_seconds = bcdToDec(Wire.read());
  RTC_minutes = bcdToDec(Wire.read());
  RTC_hours = bcdToDec(Wire.read() & 0b111111); //24 hour time
  RTC_day_of_week = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  RTC_day = bcdToDec(Wire.read());
  RTC_month = bcdToDec(Wire.read());
  RTC_year = bcdToDec(Wire.read());
}

word doEditBlink(int pos)
{
  if (!BlinkUp) return 0;
  if (!BlinkDown) return 0;
  //if (pos==5) return 0xFFFF; //need to be deleted for testing purpose only!
  int lowBit=blinkMask>>pos;
  lowBit=lowBit&B00000001;
  
  static unsigned long lastTimeEditBlink=millis();
  static bool blinkState=false;
  word mask=0;
  static int tmp=0;//blinkMask;
  if ((millis()-lastTimeEditBlink)>300) 
  {
    lastTimeEditBlink=millis();
    blinkState=!blinkState;
    /*Serial.print("blinkpattern= ");
    Serial.println(blinkPattern[menuPosition]);
    if (((blinkPattern[menuPosition]>>8)&1==1) && blinkState==true) digitalWrite(pinLowerDots, HIGH);
      else digitalWrite(pinLowerDots, LOW);
    if (((blinkPattern[menuPosition]>>7)&1==1) && blinkState==true) digitalWrite(pinUpperDots, HIGH);
      else digitalWrite(pinUpperDots, LOW);
    */
    if (blinkState) tmp= 0;
      else tmp=blinkMask;
  }
  if (((dotPattern&~tmp)>>6)&1==1) digitalWrite(pinLowerDots, HIGH);
      else digitalWrite(pinLowerDots, LOW);
  if (((dotPattern&~tmp)>>7)&1==1) digitalWrite(pinUpperDots, HIGH);
      else digitalWrite(pinUpperDots, LOW);
      
  if ((blinkState==true) && (lowBit==1)) mask=0xFFFF;//mask=B11111111;
  return mask;
}

int extractDigits(byte b)
{
  String tmp="1";
  /*Serial.print("blink pattern= ");
  Serial.println(b);
  Serial.print("stringToDisplay= ");
  Serial.println(stringToDisplay);*/
  if (b==B00000011) 
    {
    tmp=stringToDisplay.substring(0,2);
    /*Serial.print("stringToDisplay1= ");
    Serial.println(stringToDisplay);*/
    }
  if (b==B00001100) 
    {
    tmp=stringToDisplay.substring(2,4);
    /*Serial.print("stringToDisplay2= ");
    Serial.println(stringToDisplay);*/
    }
  if (b==B00110000) 
    {
      tmp=stringToDisplay.substring(4);
      /*Serial.print("stringToDisplay3= ");
      Serial.println(stringToDisplay);*/
    }
  /*Serial.print("stringToDisplay4= ");
  Serial.println(stringToDisplay);*/
  return tmp.toInt();
}

void injectDigits(byte b, int value)
{
  if (b==B00000011) stringToDisplay=PreZero(value)+stringToDisplay.substring(2);
  if (b==B00001100) stringToDisplay=stringToDisplay.substring(0,2)+PreZero(value)+stringToDisplay.substring(4);
  if (b==B00110000) stringToDisplay=stringToDisplay.substring(0,4)+PreZero(value);
}

bool isValidDate()
{
  int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  if (value[DateYearIndex]%4==0) days[1]=29;
  if (value[DateDayIndex]>days[value[DateMonthIndex]-1]) return false;
    else return true;
  
}

byte default_dur = 4;
byte default_oct = 6;
int bpm = 63;
int num;
long wholenote;
long duration;
byte note;
byte scale;
char* parseSong(char *p)
{
  // Absolutely no error checking in here
  // format: d=N,o=N,b=NNN:
  // find the start (skip name, etc)

  while(*p != ':') p++;    // ignore name
  p++;                     // skip ':'

  // get default duration
  if(*p == 'd')
  {
    p++; p++;              // skip "d="
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    if(num > 0) default_dur = num;
    p++;                   // skip comma
  }

  // get default octave
  if(*p == 'o')
  {
    p++; p++;              // skip "o="
    num = *p++ - '0';
    if(num >= 3 && num <=7) default_oct = num;
    p++;                   // skip comma
  }

  // get BPM
  if(*p == 'b')
  {
    p++; p++;              // skip "b="
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    bpm = num;
    p++;                   // skip colon
  }

  // BPM usually expresses the number of quarter notes per minute
  wholenote = (60 * 1000L / bpm) * 4;  // this is the time for whole note (in milliseconds)
  return p;
}

 // now begin note loop
 static unsigned long lastTimeNotePlaying=0;
 char* playmusic(char *p)
  {
     if(*p==0) 
      {
        return p;
      }
    if (millis()-lastTimeNotePlaying>duration) 
        lastTimeNotePlaying=millis();
      else return p;
    // first, get note duration, if available
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    
    if(num) duration = wholenote / num;
    else duration = wholenote / default_dur;  // we will need to check if we are a dotted note after

    // now get the note
    note = 0;

    switch(*p)
    {
      case 'c':
        note = 1;
        break;
      case 'd':
        note = 3;
        break;
      case 'e':
        note = 5;
        break;
      case 'f':
        note = 6;
        break;
      case 'g':
        note = 8;
        break;
      case 'a':
        note = 10;
        break;
      case 'b':
        note = 12;
        break;
      case 'p':
      default:
        note = 0;
    }
    p++;

    // now, get optional '#' sharp
    if(*p == '#')
    {
      note++;
      p++;
    }

    // now, get optional '.' dotted note
    if(*p == '.')
    {
      duration += duration/2;
      p++;
    }
  
    // now, get scale
    if(isdigit(*p))
    {
      scale = *p - '0';
      p++;
    }
    else
    {
      scale = default_oct;
    }

    scale += OCTAVE_OFFSET;

    if(*p == ',')
      p++;       // skip comma for next note (or we may be at the end)

    // now play the note

    if(note)
    {
      tone1.play(notes[(scale - 4) * 12 + note], duration);
      if (millis()-lastTimeNotePlaying>duration) 
        lastTimeNotePlaying=millis();
      else return p;
        tone1.stop();
    }
    else
    {
      return p;
    }
    Serial.println(F("Incorrect Song Format!"));
    return 0; //error
  }
  
  
void incrementValue()
  {
   enteringEditModeTime=millis();
      if (editMode==true)
      {
       if(menuPosition!=hModeValueIndex) // 12/24 hour mode menu position
       value[menuPosition]=value[menuPosition]+1; else value[menuPosition]=value[menuPosition]+12;
       if (value[menuPosition]>maxValue[menuPosition])  value[menuPosition]=minValue[menuPosition];
       if (menuPosition==Alarm01) 
        {
         if (value[menuPosition]==1) /*digitalWrite(pinUpperDots, HIGH);*/dotPattern=B10000000;//turn on all dots
           /*else digitalWrite(pinUpperDots, LOW); */ dotPattern=B00000000; //turn off all dots
        }
      injectDigits(blinkMask, value[menuPosition]);
      } 
  }
  
void dicrementValue()
{
      enteringEditModeTime=millis();
      if (editMode==true)
      {
        if (menuPosition!=hModeValueIndex) value[menuPosition]=value[menuPosition]-1; else value[menuPosition]=value[menuPosition]-12;
      if (value[menuPosition]<minValue[menuPosition]) value[menuPosition]=maxValue[menuPosition];
      if (menuPosition==Alarm01) 
        {
         if (value[menuPosition]==1) /*digitalWrite(pinUpperDots, HIGH);*/ dotPattern=B10000000;//turn on upper dots включаем верхние точки
           else /*digitalWrite(pinUpperDots, LOW);*/ dotPattern=B00000000; //turn off upper dots
        }
      injectDigits(blinkMask, value[menuPosition]);
      }
}

bool Alarm1SecondBlock=false;
unsigned long lastTimeAlarmTriggired=0;
void checkAlarmTime()
{
 if (value[Alarm01]==0) return;
 if ((Alarm1SecondBlock==true) && ((millis()-lastTimeAlarmTriggired)>1000)) Alarm1SecondBlock=false; 
 if (Alarm1SecondBlock==true) return;
 if ((hour()==value[AlarmHourIndex]) && (minute()==value[AlarmMinuteIndex]) && (second()==value[AlarmSecondIndex]))
   {
     lastTimeAlarmTriggired=millis();
     Alarm1SecondBlock=true;
     Serial.println(F("Wake up, Neo!"));
     p=song;
   }
}


void setLEDsFromEEPROM()
{
  digitalWrite(RedLedPin, EEPROM.read(LEDsRedValueEEPROMAddress));
  digitalWrite(GreenLedPin, EEPROM.read(LEDsGreenValueEEPROMAddress));
  digitalWrite(BlueLedPin, EEPROM.read(LEDsBlueValueEEPROMAddress));
}


