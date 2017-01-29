const String FirmwareVersion="010500";
//Format                _X.XX__    
//NIXIE CLOCK SHIELD NCS314 by GRA & AFCH (fominalec@gmail.com)
//1.05
//Fixed bug with blanking algorithm
//Re-implemented color rotation - blue LED wasn't cycling because of conflict with tone library.
//1.04
//Added display blanking setting (actually display on setting)
//Fixed problem with stopping music
//1.03
//Added ACP
//Added transitions for displaying temporary information such as date
//Created globals for time so that reading ms is the same everywhere for one call of loop()
//1.02 17.10.2016
//Fixed: RGB color controls
//Update to Arduino IDE 1.6.12 (Time.h replaced to TimeLib.h)
//1.01
//Added RGB LEDs lock(by UP and Down Buttons)
//Added Down and Up buttons pause and resume self testing
//25.09.2016 update to HW ver 1.1
//25.05.2016

//#define PLOT_LEDS
#define INCLUDE_TONES
#define WIFI
//#define ONE_TWO

#include <SPI.h>
#include <Wire.h>
#include <ClickButton.h>
#include <TimeLib.h>
#ifdef INCLUDE_TONES
#include <Tone.h>
#endif
#include <EEPROM.h>

#ifdef WIFI
#include "I2CClient.h"

#define I2C_ME 0x23
#endif

#ifdef ONE_TWO
const byte LEpin=10; //pin Latch Enabled data accepted while HI level
#else
const byte LEpin=7; //pin Latch Enabled data accepted while HI level
#endif
const byte DHVpin=5; // off/on MAX1771 Driver  High Voltage(DHV) 110-220V
const byte RedLedPin=9; //MCU WDM output for red LEDs 9-g, timer 1, phase correct 490.2hz.
const byte GreenLedPin=6; //MCU WDM output for green LEDs 6-b, timer 0, fast PWM 976hz. Also runs millis() and delay() so leave this alone!
const byte BlueLedPin=3; //MCU WDM output for blue LEDs 3-r, timer 2 - tone library, phase correct 490.2hz
const byte pinSet=A0;
const byte pinUp=A2;
const byte pinDown=A1;
const byte pinBuzzer=2;
const byte pinUpperDots=12; //HIGH value light a dots
const byte pinLowerDots=8;  //HIGH value light a dots
const word fpsLimit=16666; // 1/60*1.000.000 //limit maximum refresh rate on 60 fps

String stringToDisplay="000000";// Content of this string will be displayed on tubes (must be 6 chars length)
byte menuPosition=0; // 0 - time
                    // 1 - date
                    // 2 - alarm
                    // 3 - 12/24 hours mode
                    
byte blinkMask=B00000000; //bit mask for blinking digits (1 - blink, 0 - constant light)

//                      0      1      2      3      4      5      6      7      8       9
word SymbolArray[10]={65534, 65533, 65531, 65527, 65519, 65503, 65471, 65407, 65279, 65023};

byte dotPattern=B00000000; //bit mask for separating dots
                          //B00000000 - turn off up and down dots 
                          //B1100000 - turn off all dots

#define DS1307_ADDRESS 0x68
byte zero = 0x00; //workaround for issue #527
int RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year, RTC_day_of_week;

#define TimeIndex        0 
#define DateIndex        1 
#define AlarmIndex       2 
#define hModeIndex       3
#define BlankingIndex    4
#define TimeHoursIndex   5
#define TimeMintuesIndex 6
#define TimeSecondsIndex 7
#define DateDayIndex     8
#define DateMonthIndex   9
#define DateYearIndex    10
#define AlarmHourIndex   11
#define AlarmMinuteIndex 12
#define AlarmSecondIndex 13
#define AlarmArmedIndex  14
#define LeadingZeroIndex 15
#define hModeValueIndex  16
#define DateFormatIndex  17
#define DigitsOnIndex    18
#define DigitsOffIndex   19

#define FirstParent      TimeIndex
#define LastParent       BlankingIndex
#define SettingsCount    (DigitsOffIndex+1)
#define NoParent         0

#define DF_DDMMYY 0
#define DF_MMDDYY 1
#define DF_YYMMDD 2

byte dateIndexLookup[3][3] = {
    {DateDayIndex, DateMonthIndex, DateYearIndex},
    {DateMonthIndex, DateDayIndex, DateYearIndex},
    {DateYearIndex, DateMonthIndex, DateDayIndex}
};

//-------------------------------------0---------------1--------------2---------------3--------------4-------------5-----------6----------7-----------8-----------9-----------10-----------11-----------12-----------13-----------14----------15-----------16------------17-------------18--------------19
//         names:                    Time,           Date,          Alarm,          12/24          Blanking      hours,     minutes,   seconds,      day,        month,      year,        hour,       minute,      second       alarm01    leading_zero hour_format   date_format    on hour         off hour
//                                     1               1              1               1              1             1           1          1           1           1            1            1            1            1            1           1            1             1               1              1
byte parent[SettingsCount]=    {   NoParent,       NoParent,      NoParent,       NoParent,     NoParent,    TimeIndex+1,TimeIndex+1,TimeIndex+1,DateIndex+1,DateIndex+1,DateIndex+1,AlarmIndex+1,AlarmIndex+1,AlarmIndex+1,AlarmIndex+1,hModeIndex+1,hModeIndex+1,hModeIndex+1,BlankingIndex+1,BlankingIndex+1};
byte firstChild[SettingsCount]={TimeHoursIndex,  DateDayIndex, AlarmHourIndex,LeadingZeroIndex,DigitsOnIndex,      0,          0,         0,          0,          0,           0,           0,           0,           0,           0,          0,           0,            0,            0,              0};
byte lastChild[SettingsCount]= {TimeSecondsIndex,DateYearIndex,AlarmArmedIndex,DateFormatIndex,DigitsOffIndex,     0,          0,         0,          0,          0,           0,           0,           0,           0,           0,          0,           0,            0,            0,              0};
char value[SettingsCount]=     {       0,              0,             0,              0,             0,            0,          0,         0,          0,          0,           0,           0,           0,           0,           0,          0,          24,            0,            0,              0};
byte maxValue[SettingsCount]=  {       0,              0,             0,              0,             0,           23,         59,        59,         31,         12,          99,          23,          59,          59,           1,          1,          24,            2,           23,             23};
byte minValue[SettingsCount]=  {       0,              0,             0,             12,             0,           00,         00,        00,          1,          1,          00,          00,          00,          00,           0,          0,          12,            0,            0,              0};
byte blinkPattern[SettingsCount]={
                                  B00000000,
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
                                                                                                                                                                                                                                         B00000011,
                                                                                                                                                                                                                                                        B00001100,
                                                                                                                                                                                                                                                                     B00110000,
                                                                                                                                                                                                                                                                                    B00000011,
                                                                                                                                                                                                                                                                                                   B00110000};
bool editMode=false;

// If true, blank that digit H,H,M,M,S,S
byte blanks[6] = {false, false, false, false, false, false};
// Global ms (so it is the same in all methods called from loop()
unsigned long nowMillis;
// Global System time for one run of the loop h, m, s, ms (so they are the same in every method called from loop()
unsigned int GLOB_TIME[4] = {0, 0, 0, 0};
#define HOUR_IDX 0
#define MIN_IDX 1
#define SEC_IDX 2
#define MS_IDX 3

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
byte HueEEPROMAddress=8;
byte LEDsLockEEPROMAddress=7;
byte DigitsOnEEPROMAddress=11;
byte DigitsOffEEPROMAddress=12;
byte LeadingZeroEEPROMAddress=13;
byte DateFormatEEPROMAddress=14;
byte CycleTimeEEPROMAddress=15;
byte SaturationEEPROMAddress=16;
byte BrightnessEEPROMAddress=17;

//buttons pins declarations
ClickButton setButton(pinSet, LOW, CLICKBTN_PULLUP);
ClickButton upButton(pinUp, LOW, CLICKBTN_PULLUP);
ClickButton downButton(pinDown, LOW, CLICKBTN_PULLUP);
///////////////////
#ifdef INCLUDE_TONES
Tone tone1;
#define isdigit(n) (n >= '0' && n <= '9')
//char *song = "MissionImp:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,a#,g,2d,32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d";
//char *song = "PinkPanther:d=4,o=5,b=160:8d#,8e,2p,8f#,8g,2p,8d#,8e,16p,8f#,8g,16p,8c6,8b,16p,8d#,8e,16p,8b,2a#,2p,16a,16g,16e,16d,2e";
//char *song="VanessaMae:d=4,o=6,b=70:32c7,32b,16c7,32g,32p,32g,32p,32d#,32p,32d#,32p,32c,32p,32c,32p,32c7,32b,16c7,32g#,32p,32g#,32p,32f,32p,16f,32c,32p,32c,32p,32c7,32b,16c7,32g,32p,32g,32p,32d#,32p,32d#,32p,32c,32p,32c,32p,32g,32f,32d#,32d,32c,32d,32d#,32c,32d#,32f,16g,8p,16d7,32c7,32d7,32a#,32d7,32a,32d7,32g,32d7,32d7,32p,32d7,32p,32d7,32p,16d7,32c7,32d7,32a#,32d7,32a,32d7,32g,32d7,32d7,32p,32d7,32p,32d7,32p,32g,32f,32d#,32d,32c,32d,32d#,32c,32d#,32f,16c";
//char *song="DasBoot:d=4,o=5,b=100:d#.4,8d4,8c4,8d4,8d#4,8g4,a#.4,8a4,8g4,8a4,8a#4,8d,2f.,p,f.4,8e4,8d4,8e4,8f4,8a4,c.,8b4,8a4,8b4,8c,8e,2g.,2p";
//char *song="Scatman:d=4,o=5,b=200:8b,16b,32p,8b,16b,32p,8b,2d6,16p,16c#.6,16p.,8d6,16p,16c#6,8b,16p,8f#,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8f#,2p,32p,2d6,16p,16c#6,8p,16d.6,16p.,16c#6,16a.,16p.,8e,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8b,16b,32p,8b,16b,32p,8b,2d6,16p,16c#.6,16p.,8d6,16p,16c#6,8b,16p,8f#,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8f#,2p,32p,2d6,16p,16c#6,8p,16d.6,16p.,16c#6,16a.,16p.,8e,2p.,16c#6,8p,16d.6,16p.,16c#6,16a,8p,8e,2p,32p,16f#.6,16p.,16b.,16p.";
char *song="Popcorn:d=4,o=5,b=160:8c6,8a#,8c6,8g,8d#,8g,c,8c6,8a#,8c6,8g,8d#,8g,c,8c6,8d6,8d#6,16c6,8d#6,16c6,8d#6,8d6,16a#,8d6,16a#,8d6,8c6,8a#,8g,8a#,c6";
//char *song="WeWishYou:d=4,o=5,b=200:d,g,8g,8a,8g,8f#,e,e,e,a,8a,8b,8a,8g,f#,d,d,b,8b,8c6,8b,8a,g,e,d,e,a,f#,2g,d,g,8g,8a,8g,8f#,e,e,e,a,8a,8b,8a,8g,f#,d,d,b,8b,8c6,8b,8a,g,e,d,e,a,f#,1g,d,g,g,g,2f#,f#,g,f#,e,2d,a,b,8a,8a,8g,8g,d6,d,d,e,a,f#,2g";
#define OCTAVE_OFFSET 0
char *p;

int notes[] = { 0,
NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7
};
#endif

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w=1);

boolean checkDisplay(boolean override=false);
String getTimeString(boolean forceUpdate=false);
String getDateString();

int functionDownButton=0;
int functionUpButton=0;
bool LEDsLock=false;
byte ledCycleTime = 3;
byte hue=0;
byte saturation = 255;
byte brightness = 255;

struct Transition {
  /**
   * Default the effect and hold duration.
   */
  Transition() : Transition(500, 500, 3000) {
  }

  Transition(int effectDuration, int holdDuration) : Transition(effectDuration, effectDuration, holdDuration) {
  }

  Transition(int effectInDuration, int effectOutDuration, int holdDuration) {
    this->effectInDuration = effectInDuration;
    this->effectOutDuration = effectOutDuration;
    this->holdDuration = holdDuration;
    this->started = 0;
    this->end = 0;
  }

  void start(long now) {
    if (end < now) {
      this->started = now;
      this->end = getEnd();
    }
    // else we are already running!
  }

  boolean scrollMsg(long now, void (*loadRegularValues)(), void (*loadMessageValues)())
  {
    if (now < end) {
      int msCount = now - started;
      if (msCount < effectInDuration) {
        loadRegularValues();
        // Scroll -1 -> -6
        scroll(-(msCount % effectInDuration) * 6 / effectInDuration - 1);
      } else if (msCount < effectInDuration * 2) {
        loadMessageValues();
        // Scroll 5 -> 0
        scroll(5 - (msCount % effectInDuration) * 6 / effectInDuration);
      } else if (msCount < effectInDuration * 2 + holdDuration) {
        loadMessageValues();
      } else if (msCount < effectInDuration * 2 + holdDuration + effectOutDuration) {
        loadMessageValues();
        // Scroll 1 -> 6
        scroll(((msCount - holdDuration) % effectOutDuration) * 6 / effectOutDuration + 1);
      } else if (msCount < effectInDuration * 2 + holdDuration + effectOutDuration * 2) {
        loadRegularValues();
        // Scroll 0 -> -5
        scroll(((msCount - holdDuration) % effectOutDuration) * 6 / effectOutDuration - 5);
      }

      return true;  // we are still running
    }

    return false;   // We aren't running
  }

  boolean scrambleMsg(long now, void (*loadRegularValues)(), void (*loadMessageValues)())
  {
    if (now < end) {
      int msCount = now - started;
      if (msCount < effectInDuration) {
        loadRegularValues();
        scramble(msCount, 5 - (msCount % effectInDuration) * 6 / effectInDuration, 6);
      } else if (msCount < effectInDuration * 2) {
        loadMessageValues();
        scramble(msCount, 0, 5 - (msCount % effectInDuration) * 6 / effectInDuration);
      } else if (msCount < effectInDuration * 2 + holdDuration) {
        loadMessageValues();
      } else if (msCount < effectInDuration * 2 + holdDuration + effectOutDuration) {
        loadMessageValues();
        scramble(msCount, 0, ((msCount - holdDuration) % effectOutDuration) * 6 / effectOutDuration + 1);
      } else if (msCount < effectInDuration * 2 + holdDuration + effectOutDuration * 2) {
        loadRegularValues();
        scramble(msCount, ((msCount - holdDuration) % effectOutDuration) * 6 / effectOutDuration + 1, 6);
      }

      return true;  // we are still running
    }

    return false;   // We aren't running
  }

  boolean scrollInScrambleOut(long now, void (*loadRegularValues)(), void (*loadMessageValues)())
  {
    if (now < end) {
      int msCount = now - started;
      if (msCount < effectInDuration) {
        loadRegularValues();
        scroll(-(msCount % effectInDuration) * 6 / effectInDuration - 1);
      } else if (msCount < effectInDuration * 2) {
        loadMessageValues();
        scroll(5 - (msCount % effectInDuration) * 6 / effectInDuration);
      } else if (msCount < effectInDuration * 2 + holdDuration) {
        loadMessageValues();
      } else if (msCount < effectInDuration * 2 + holdDuration + effectOutDuration) {
        loadMessageValues();
        scramble(msCount, 0, ((msCount - holdDuration) % effectOutDuration) * 6 / effectOutDuration + 1);
      } else if (msCount < effectInDuration * 2 + holdDuration + effectOutDuration * 2) {
        loadRegularValues();
        scramble(msCount, ((msCount - holdDuration) % effectOutDuration) * 6 / effectOutDuration + 1, 6);
      }

      return true;  // we are still running
    }

    return false;   // We aren't running
  }

  static void loadTime() {
    stringToDisplay = getTimeString(true);
  }

  static void loadDate() {
    stringToDisplay = getDateString();
  }

  /**
   * +ve scroll right
   * -ve scroll left
   */
  static int scroll(char count) {
    String copy = stringToDisplay;
    char offset = 0;
    char slope = 1;
    if (count < 0) {
      count = -count;
      offset = 5;
      slope = -1;
    }
    for (char i=0; i<6; i++) {
      blanks[offset + i * slope] = i < count;
      if (i >= count) {
        stringToDisplay[offset + i * slope] = copy[offset + (i-count) * slope];
      }
    }

    return count;
  }

  static long hash(long x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
  }

  // In these functions we want something that changes every 100ms,
  // hence msCount/100. Plus it needs to be different for different
  // indices, hence +i. Plus it needs to be 'random', hence hash function
  static int scramble(int msCount, byte start, byte end) {
    for (int i=start; i < end; i++) {
      stringToDisplay[i] = '0' + hash(msCount / 100 + i) % 10;
    }

    return start;
  }

protected:
  /**** Don't let Joe Public see this stuff ****/

  int effectInDuration; // How long an effect should take in ms
  int effectOutDuration; // How long an effect should take in ms
  int holdDuration;   // How long the message should be displayed for in ms
  long started;       // When we were started (timestamp)
  long end;           // When the whole thing will end (timestamp)

  /**
   * The end time has to match what displayMessage() wants,
   * so let sub-classes override it.
   */
  long getEnd() {
    return started + effectInDuration * 2 + holdDuration + effectOutDuration * 2;
  }
};

Transition transition(500, 1000, 3000);

void setTickGlobals() {
  nowMillis = millis();
  GLOB_TIME[HOUR_IDX] = hour();
  GLOB_TIME[MIN_IDX] = minute();
  GLOB_TIME[SEC_IDX] = second();
  GLOB_TIME[MS_IDX] = nowMillis % 1000;
}

/*******************************************************************************************************
Init Program
*******************************************************************************************************/
void setup() 
{
  setTickGlobals();

  digitalWrite(DHVpin, LOW);    // off MAX1771 Driver  High Voltage(DHV) 110-220V

  // Initialize from clock in case WiFi is down on startup
  Wire.begin();
  //setRTCDateTime(23,40,00,25,7,15,1);
  
  Serial.begin(115200);
  
    if (EEPROM.read(HourFormatEEPROMAddress)!=12) value[hModeValueIndex]=24; else value[hModeValueIndex]=12;
    if (EEPROM.read(LeadingZeroEEPROMAddress)==255) value[LeadingZeroIndex]=1; else value[LeadingZeroIndex]=EEPROM.read(LeadingZeroEEPROMAddress) % 2;
    if (EEPROM.read(DateFormatEEPROMAddress)==255) value[DateFormatIndex]=0; else value[DateFormatIndex]=EEPROM.read(DateFormatEEPROMAddress) % 3;
    if (EEPROM.read(CycleTimeEEPROMAddress)==255) ledCycleTime = 5; else ledCycleTime=EEPROM.read(CycleTimeEEPROMAddress) % 200 + 1;
    if (EEPROM.read(RGBLEDsEEPROMAddress)!=0) RGBLedsOn=true; else RGBLedsOn=false;
    if (EEPROM.read(AlarmTimeEEPROMAddress)==255) value[AlarmHourIndex]=0; else value[AlarmHourIndex]=EEPROM.read(AlarmTimeEEPROMAddress);
    if (EEPROM.read(AlarmTimeEEPROMAddress+1)==255) value[AlarmMinuteIndex]=0; else value[AlarmMinuteIndex]=EEPROM.read(AlarmTimeEEPROMAddress+1);
    if (EEPROM.read(AlarmTimeEEPROMAddress+2)==255) value[AlarmSecondIndex]=0; else value[AlarmSecondIndex]=EEPROM.read(AlarmTimeEEPROMAddress+2);
    if (EEPROM.read(AlarmArmedEEPROMAddress)==255) value[AlarmArmedIndex]=0; else value[AlarmArmedIndex]=EEPROM.read(AlarmArmedEEPROMAddress);
    if (EEPROM.read(LEDsLockEEPROMAddress)==255) LEDsLock=false; else LEDsLock=EEPROM.read(LEDsLockEEPROMAddress);
    if (EEPROM.read(DigitsOnEEPROMAddress)==255) value[DigitsOnIndex]=0; else value[DigitsOnIndex]=EEPROM.read(DigitsOnEEPROMAddress);
    if (EEPROM.read(DigitsOffEEPROMAddress)==255) value[DigitsOffIndex]=0; else value[DigitsOffIndex]=EEPROM.read(DigitsOffEEPROMAddress);

    hue = EEPROM.read(HueEEPROMAddress);
    saturation = EEPROM.read(SaturationEEPROMAddress);
    brightness = EEPROM.read(BrightnessEEPROMAddress);

    Serial.print("led lock=");
    Serial.println(LEDsLock);
    
  pinMode(RedLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(BlueLedPin, OUTPUT);
#ifdef INCLUDE_TONES
  tone1.begin(pinBuzzer);
  song=parseSong(song);
#endif
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
      setHueFromEEPROM();
    }

  // Initialize with time from RTC in case WiFi is down
  getRTCTime();
  setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
  digitalWrite(DHVpin, LOW); // off MAX1771 Driver  High Voltage(DHV) 110-220V
  setRTCDateTime(RTC_hours,RTC_minutes,RTC_seconds,RTC_day,RTC_month,RTC_year,1); //Ð·Ð°Ð¿Ð¸Ñ�Ñ‹Ð²Ð°ÐµÐ¼ Ñ‚Ð¾Ð»ÑŒÐºÐ¾ Ñ‡Ñ‚Ð¾ Ñ�Ñ‡Ð¸Ñ‚Ð°Ð½Ð½Ð¾Ðµ Ð²Ñ€ÐµÐ¼Ñ� Ð² RTC Ñ‡Ñ‚Ð¾Ð±Ñ‹ Ð·Ð°Ð¿ÑƒÑ�Ñ‚Ð¸Ñ‚ÑŒ Ð½Ð¾Ð²ÑƒÑŽ Ð¼Ð¸ÐºÑ€Ð¾Ñ�Ñ…ÐµÐ¼Ñƒ
  digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  High Voltage(DHV) 110-220V
  setTickGlobals();

#ifdef WIFI
  // Connect to WiFi instead of RTC
  Wire.begin(I2C_ME);
  Wire.onReceive(receiveHandler);
  Wire.onRequest(requestHandler);
#endif

  //p=song;
}

void rotateLeft(uint8_t &bits)
{
   uint8_t high_bit = bits & (1 << 7) ? 1 : 0;
  bits = (bits << 1) | high_bit;
}

byte RedLight=255;
byte GreenLight=0;
byte BlueLight=0;
unsigned long prevTime=0; // time of lase tube was lit
unsigned long prevTime4FireWorks=0;  //time of last RGB changed
//int minuteL=0; //младшая цифра минут

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
void loop() {
  setTickGlobals();
#ifdef INCLUDE_TONES
  disableTimer();
  p=playmusic(p);
#endif
  rotateFireWorks(); //change color (by 1 step)

  doIndication();
  
  clearBlanks(false);

  setButton.Update();
  upButton.Update();
  downButton.Update();
  if (editMode==false) 
    {
      blinkMask=B00000000;
    } else if ((nowMillis-enteringEditModeTime)>60000)
    {
      editMode=false;
      menuPosition=firstChild[menuPosition];
      blinkMask=blinkPattern[menuPosition];
    }
  if (setButton.clicks>0) //short click, advance menu position by 1
    {
      if (digitalRead(DHVpin) == LOW) {
        // If display is turned off, just turn it back on again
        checkDisplay(true);
      } else {
        // Otherwise reset temp timer (just in case) and advance menu
        checkDisplay(true);
#ifdef INCLUDE_TONES
        p=0; //shut off music )))
        playTone(1000,100);
#endif
        enteringEditModeTime=nowMillis;
        menuPosition=menuPosition+1;
        if (menuPosition==LastParent+1) menuPosition=FirstParent;
        Serial.print(F("menuPosition="));
        Serial.println(menuPosition);
        Serial.print(F("value="));
        Serial.println((byte)(value[menuPosition]));

        blinkMask=blinkPattern[menuPosition];
        if ((parent[menuPosition-1]!=NoParent) and (lastChild[parent[menuPosition-1]-1]==(menuPosition-1)))
        {
        // Exiting child menu
          if ((parent[menuPosition-1]-1==DateIndex) && (!isValidDate()))
            {
              menuPosition=DateDayIndex;
              return;
            }
          editMode=false;
          menuPosition=parent[menuPosition-1]-1;
          switch (menuPosition) {
          case TimeIndex:
            setTime(value[TimeHoursIndex], value[TimeMintuesIndex], value[TimeSecondsIndex], day(), month(), year());
            break;
          case DateIndex:
            setTime(GLOB_TIME[HOUR_IDX], GLOB_TIME[MIN_IDX], GLOB_TIME[SEC_IDX],value[xlateMenuPosition(DateDayIndex)], value[xlateMenuPosition(DateMonthIndex)], 2000+value[xlateMenuPosition(DateYearIndex)]);
            break;
          case AlarmIndex:
            EEPROM.write(AlarmTimeEEPROMAddress,  value[AlarmHourIndex]);
            EEPROM.write(AlarmTimeEEPROMAddress+1,value[AlarmMinuteIndex]);
            EEPROM.write(AlarmTimeEEPROMAddress+2,value[AlarmSecondIndex]);
            EEPROM.write(AlarmArmedEEPROMAddress, value[AlarmArmedIndex]);
            break;
          case hModeIndex:
            EEPROM.write(LeadingZeroEEPROMAddress,value[LeadingZeroIndex]);
            EEPROM.write(HourFormatEEPROMAddress, value[hModeValueIndex]);
            EEPROM.write(DateFormatEEPROMAddress, value[DateFormatIndex]);
            break;
          case BlankingIndex:
            EEPROM.write(DigitsOnEEPROMAddress,  value[DigitsOnIndex]);
            EEPROM.write(DigitsOffEEPROMAddress, value[DigitsOffIndex]);
            break;
          }
          digitalWrite(DHVpin, LOW); // off MAX1771 Driver  High Voltage(DHV) 110-220V
          setRTCDateTime(hour(),minute(),second(),day(),month(),year()%1000,1);
          digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  High Voltage(DHV) 110-220V
        }
        value[menuPosition]=extractDigits(blinkMask);
      }
    }
  if (setButton.clicks<0) //long click, toggle edit mode for that menu position
    {
#ifdef INCLUDE_TONES
    playTone(1000,100);
#endif
      editMode=!editMode;
      if (editMode)
      {
        enteringEditModeTime=nowMillis;
        if (menuPosition==TimeIndex) stringToDisplay=PreZero(GLOB_TIME[HOUR_IDX])+PreZero(GLOB_TIME[MIN_IDX])+PreZero(GLOB_TIME[SEC_IDX]); //temporary enabled 24 hour format while settings
      }
      menuPosition=firstChild[menuPosition];
      if (menuPosition==AlarmHourIndex) {value[AlarmArmedIndex]=1; /*digitalWrite(pinUpperDots, HIGH);*/dotPattern=B10000000;}
      blinkMask=blinkPattern[menuPosition];
      value[menuPosition]=extractDigits(blinkMask);
    }
   
  if (upButton.clicks != 0) functionUpButton = upButton.clicks;  
   
  if (upButton.clicks>0) 
    {
      incrementValue();
      if (!editMode) 
        {
          LEDsLock=false;
          EEPROM.write(LEDsLockEEPROMAddress, 0);
        }
#ifdef INCLUDE_TONES
      p=0; //shut off music )))
      playTone(1000,100);
#endif
    }
  
  if (functionUpButton == -1 && upButton.depressed == true)   
  {
   BlinkUp=false;
   if (editMode==true) 
   {
     if ( (nowMillis - upTime) > settingDelay)
    {
     upTime = nowMillis;// + settingDelay;
      incrementValue();
    }
   }
  } else BlinkUp=true;
  
  if (downButton.clicks != 0) functionDownButton = downButton.clicks;
  
  if (downButton.clicks>0) 
    {
#ifdef INCLUDE_TONES
      p=0; //shut off music )))
      playTone(1000,100);
#endif
      decrementValue();
      if (!editMode)
      {
        LEDsLock=true;
        EEPROM.write(LEDsLockEEPROMAddress, 1);
        EEPROM.write(HueEEPROMAddress, hue);
      }
    }
  
  if (functionDownButton == -1 && downButton.depressed == true)   
  {
   BlinkDown=false;
   if (editMode==true) 
   {
     if ( (nowMillis - downTime) > settingDelay)
    {
     downTime = nowMillis;// + settingDelay;
      decrementValue();
    }
   }
  } else BlinkDown=true;
  
  if (!editMode)
  {
    if (upButton.clicks<0)  // Long click
      {
#ifdef INCLUDE_TONES
      playTone(1000,100);
#endif
        RGBLedsOn=true;
        EEPROM.write(RGBLEDsEEPROMAddress,1);
        Serial.println("RGB=on");
        setHueFromEEPROM();
      }
    if (downButton.clicks<0)  // Long click
    {
#ifdef INCLUDE_TONES
      playTone(1000,100);
#endif
      RGBLedsOn=false;
      EEPROM.write(RGBLEDsEEPROMAddress,0);
      Serial.println("RGB=off");
    }
  }
  
    
  switch (menuPosition)
  {
    case TimeIndex: //time mode
    {
      checkAlarmTime();
      if (!checkDisplay()) {
    	  break;
      }

      if (GLOB_TIME[SEC_IDX] == 50) {
        transition.start(nowMillis);
      }

      boolean msgDisplaying = transition.scrollInScrambleOut(nowMillis, Transition::loadTime, Transition::loadDate);

      if (!msgDisplaying) {
        if ((GLOB_TIME[MIN_IDX] % 10) == 0 && (GLOB_TIME[SEC_IDX] >= 0 && GLOB_TIME[SEC_IDX] <= 10)) {
          stringToDisplay = oneArmedBandit();
        } else {
          stringToDisplay = getTimeString();
          clearBlanks(true);
        }
      }
      doDotBlink();
    }
       break;
    case DateIndex: //date mode
      stringToDisplay=getDateString();
      dotPattern=B01000000;//turn on lower dots
      /*digitalWrite(pinUpperDots, LOW);
      digitalWrite(pinLowerDots, HIGH);*/
      checkAlarmTime();
      break;
    case AlarmIndex: //alarm mode
      stringToDisplay=PreZero(value[AlarmHourIndex])+PreZero(value[AlarmMinuteIndex])+PreZero(value[AlarmSecondIndex]);
     if (value[AlarmArmedIndex]==1) /*digitalWrite(pinUpperDots, HIGH);*/ dotPattern=B10000000; //turn on upper dots
           else 
           {
             /*digitalWrite(pinUpperDots, LOW);
             digitalWrite(pinLowerDots, LOW);*/
             dotPattern=B00000000; //turn off upper dots
           }
      checkAlarmTime();
      break;
    case hModeIndex: //12/24 hours mode
      stringToDisplay=PreZero(value[LeadingZeroIndex])+PreZero(value[hModeValueIndex])+PreZero(value[DateFormatIndex]);
      dotPattern=B00000000;//turn off all dots
      /*digitalWrite(pinUpperDots, LOW);
      digitalWrite(pinLowerDots, LOW);*/
      checkAlarmTime();
      break;
    case BlankingIndex: // Display blanking mode
      stringToDisplay=PreZero(value[DigitsOnIndex])+"00"+PreZero(value[DigitsOffIndex]);
      checkAlarmTime();
      break;
  }
}

#ifdef WIFI
void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void receiveHandler(int bytes) {
  byte command = Wire.read();

  if (command > ((byte)I2CCommands::alarm_time)) {
    // This is just a regular string
    Serial.print((char)command);
    while (Wire.available()) {
      char c = (char) Wire.read();
      Serial.print(c);
    }

    Serial.println("");
  } else {
    Serial.print("Received command ");
    Serial.print(command);
    byte readValue = Wire.read();
    Serial.print(", value ");
    Serial.println(readValue);
    switch ((I2CCommands) command) {
    case I2CCommands::time:
      {
        byte year = readValue;
        byte month = Wire.read();
        byte day = Wire.read();
        byte hours = Wire.read();
        byte mins = Wire.read();
        byte secs = Wire.read();
        setTime(hours, mins, secs, day, month, year);
        digitalClockDisplay();
      }
      break;
    case I2CCommands::time_or_date:
      menuPosition = readValue == 0 ? DateIndex : TimeIndex;
      break;
    case I2CCommands::date_format:
      if (readValue >= 0 && readValue <= 2) {
        value[DateFormatIndex] = readValue;
        EEPROM.write(DateFormatEEPROMAddress, readValue);
      }
      break;
    case I2CCommands::time_format:
      value[hModeValueIndex] = readValue == 0 ? 24 : 12;
      EEPROM.write(HourFormatEEPROMAddress, value[hModeValueIndex]);
      break;  //false = 24 hour
    case I2CCommands::leading_zero:
      value[LeadingZeroIndex] = readValue > 0;
      EEPROM.write(LeadingZeroEEPROMAddress, value[LeadingZeroIndex]);
      break;
    case I2CCommands::display_on:
      value[DigitsOnIndex] = readValue;
      EEPROM.write(DigitsOnEEPROMAddress,  value[DigitsOnIndex]);
      break;
    case I2CCommands::display_off:
      value[DigitsOffIndex] = readValue;
      EEPROM.write(DigitsOffEEPROMAddress, value[DigitsOffIndex]);
      break;
    case I2CCommands::backlight:
      RGBLedsOn = readValue > 0;
      EEPROM.write(RGBLEDsEEPROMAddress, RGBLedsOn);
      break;
    case I2CCommands::hue_cycling:
      LEDsLock = readValue == 0;
      EEPROM.write(LEDsLockEEPROMAddress, LEDsLock);
      break;
    case I2CCommands::cycle_time:
      if (readValue >= 1 && readValue <= 200) {
        ledCycleTime = readValue;
        EEPROM.write(CycleTimeEEPROMAddress, ledCycleTime);
      }
      break;
    case I2CCommands::hue:
      hue = readValue;
      EEPROM.write(HueEEPROMAddress, hue);
      break;
    case I2CCommands::saturation:
      saturation = readValue;
      EEPROM.write(SaturationEEPROMAddress, saturation);
      break;
    case I2CCommands::brightness:
      brightness = readValue;
      EEPROM.write(BrightnessEEPROMAddress, brightness);
      break;
    case I2CCommands::alarm_set:
      value[AlarmArmedIndex] = readValue > 0;
      EEPROM.write(AlarmArmedEEPROMAddress, value[AlarmArmedIndex]);
      break;
    case I2CCommands::alarm_time:
      if (readValue >= 0 && readValue <= 23) {
        value[AlarmHourIndex] = readValue;
        EEPROM.write(AlarmTimeEEPROMAddress,  value[AlarmHourIndex]);
      }
      readValue = Wire.read();
      Serial.print("And value ");
      Serial.println(readValue);
      if (readValue >= 0 && readValue <= 59) {
        value[AlarmMinuteIndex] = readValue;
        EEPROM.write(AlarmTimeEEPROMAddress+1,value[AlarmMinuteIndex]);
      }
      value[AlarmSecondIndex] = 0;
      EEPROM.write(AlarmTimeEEPROMAddress+2,value[AlarmSecondIndex]);
      break;
    }
  }
}

void requestHandler() {
  byte results[17];
  results[((byte)I2CCommands::time_or_date)] = menuPosition == DateIndex ? 0 : 1;
  results[((byte)I2CCommands::date_format)] = value[DateFormatIndex];
  results[((byte)I2CCommands::time_format)] = value[hModeValueIndex] == 24 ? 0 : 1;
  results[((byte)I2CCommands::leading_zero)] = value[LeadingZeroIndex];
  results[((byte)I2CCommands::display_on)] = value[DigitsOnIndex];
  results[((byte)I2CCommands::display_off)] = value[DigitsOffIndex];

  results[((byte)I2CCommands::backlight)] = RGBLedsOn;
  results[((byte)I2CCommands::hue_cycling)] = !LEDsLock;
  results[((byte)I2CCommands::cycle_time)] = ledCycleTime;
  results[((byte)I2CCommands::hue)] = hue;
  results[((byte)I2CCommands::saturation)] = saturation;
  results[((byte)I2CCommands::brightness)] = brightness;

  results[((byte)I2CCommands::alarm_set)] = value[AlarmArmedIndex];
  results[((byte)I2CCommands::alarm_time)] = value[AlarmHourIndex];
  results[((byte)I2CCommands::alarm_time)+1] = value[AlarmMinuteIndex];

  digitalWrite(DHVpin, LOW); // off MAX1771 Driver  High Voltage(DHV) 110-220V
  Wire.write(results, 17);
  digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  High Voltage(DHV) 110-220V
}
#endif

String PreZero(int digit)
{
  if (digit<10) return String("0")+String(digit);
    else return String(digit);
}

void setLedColorHSV(byte h, byte s, byte v) {
  // this is the algorithm to convert from RGB to HSV
  h = (h * 192) / 256;  // 0..191
  unsigned int  i = h / 32;   // We want a value of 0 thru 5
  unsigned int f = (h % 32) * 8;   // 'fractional' part of 'i' 0..248 in jumps

  unsigned int sInv = 255 - s;  // 0 -> 0xff, 0xff -> 0
  unsigned int fInv = 255 - f;  // 0 -> 0xff, 0xff -> 0
  byte pv = v * sInv / 256;  // pv will be in range 0 - 255
  byte qv = v * (256 - s * f / 256) / 256;
  byte tv = v * (256 - s * fInv / 256) / 256;

  switch (i) {
  case 0:
    RedLight = v;
    GreenLight = tv;
    BlueLight = pv;
    break;
  case 1:
    RedLight = qv;
    GreenLight = v;
    BlueLight = pv;
    break;
  case 2:
    RedLight = pv;
    GreenLight = v;
    BlueLight = tv;
    break;
  case 3:
    RedLight = pv;
    GreenLight = qv;
    BlueLight = v;
    break;
  case 4:
    RedLight = tv;
    GreenLight = pv;
    BlueLight = v;
    break;
  case 5:
    RedLight = v;
    GreenLight = pv;
    BlueLight = qv;
    break;
  }
}

void rotateFireWorks()
{
  if (!RGBLedsOn || digitalRead(DHVpin) == LOW)
  {
    analogWrite(RedLedPin,0 );
    analogWrite(GreenLedPin,0);
    analogWrite(BlueLedPin,0); 
    return;
  }
  // Approximately 20ms -> 6000ms, or complete cycle time of 6s to 1536s (about 25 mins)
  if ((nowMillis-prevTime4FireWorks)>=(ledCycleTime * 6L * 1000) / 256)
  {
    prevTime4FireWorks=nowMillis;
    setLedColorHSV(hue, saturation, brightness);
    analogWrite(RedLedPin,RedLight);
    analogWrite(GreenLedPin,GreenLight);
    analogWrite(BlueLedPin,BlueLight);
    if (!LEDsLock) {
      // Doing it this way keeps the timing the same as for rotating colors. This
      // is important to make sure the color is the same!
      hue++;
    }
  #ifdef PLOT_LEDS
    plotLEDs();
  #endif
  }
}

String oneArmedBandit() {
  static long lastTime = 0;

  if (nowMillis - lastTime > 100) {
    lastTime = nowMillis;
    return String(random(1000000));
  }

  return stringToDisplay;
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
  if ((digitalRead(pinSet)==0)||(digitalRead(pinUp)==0)||(digitalRead(pinDown)==0))
  {
    if (buttonsWasChecked==false) startBuzzTime=nowMillis;
    buttonsWasChecked=true;
  } else buttonsWasChecked=false;
  if (nowMillis-startBuzzTime<30)
    {
      digitalWrite(pinBuzzer, HIGH);
    } else
    {
      digitalWrite(pinBuzzer, LOW);
    }
}

int get12Hour() {
  if (GLOB_TIME[HOUR_IDX] == 0)
    return 12;
  else if (GLOB_TIME[HOUR_IDX] <= 12)
    return GLOB_TIME[HOUR_IDX];
  else
    return GLOB_TIME[HOUR_IDX] - 12;
}

String getTimeString(boolean forceUpdate)
{
  static unsigned int lastTimeStringWasUpdated = GLOB_TIME[SEC_IDX];
  if (lastTimeStringWasUpdated != GLOB_TIME[SEC_IDX] || forceUpdate)
  {
    //Serial.println("doDotBlink");
    //doDotBlink();
    lastTimeStringWasUpdated=GLOB_TIME[SEC_IDX];
    if (value[hModeValueIndex]==24) return PreZero(GLOB_TIME[HOUR_IDX])+PreZero(GLOB_TIME[MIN_IDX])+PreZero(GLOB_TIME[SEC_IDX]);
      else return PreZero(get12Hour())+PreZero(GLOB_TIME[MIN_IDX])+PreZero(GLOB_TIME[SEC_IDX]);
    
  }
  return stringToDisplay;
}

String getDateString()
{
  switch(value[DateFormatIndex]) {
  case DF_MMDDYY:
    return PreZero(month())+PreZero(day())+PreZero(year()%1000);
  case DF_YYMMDD:
    return PreZero(year()%1000)+PreZero(month())+PreZero(day());
  default:
    return PreZero(day())+PreZero(month())+PreZero(year()%1000);
  }
}

void doTest()
{
  Serial.print(F("Firmware version: "));
  Serial.println(FirmwareVersion.substring(1,2)+"."+FirmwareVersion.substring(2,4));
  Serial.println(F("Start Test"));
  int adc=analogRead(A3);
  float Uinput=4.6*(5.0*adc)/1024.0+0.7;
  Serial.print(F("U input="));
  Serial.print(Uinput);
#ifdef INCLUDE_TONES
  p=song;
  parseSong(p);
#endif
   
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

  unsigned int dlay=500;
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
  static unsigned long lastTimeBlink=GLOB_TIME[SEC_IDX];
  static bool dotState=0;

  if (lastTimeBlink != GLOB_TIME[SEC_IDX])
  {
    lastTimeBlink=GLOB_TIME[SEC_IDX];;
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

    if (value[AlarmArmedIndex]) {
      dotPattern^=B01000000; // If alarm is set, alternate upper/lower dots
    }
  }
}

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w)
{
#ifdef WIFI
  Wire.begin();
#endif

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

#ifdef WIFI
  // Connect to WiFi instead of RTC
  Wire.begin(I2C_ME);
  Wire.onReceive(receiveHandler);
  Wire.onRequest(requestHandler);
#endif
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


void clearBlanks(boolean timeDisplay) {
  for (int i=0; i<6; i++) {
    blanks[i] = false;
  }

  if (timeDisplay && stringToDisplay.charAt(0) == '0' && value[LeadingZeroIndex] == 0) {
    blanks[0] = true;
  }
}

word doEditBlink(int pos)
{
  if (blanks[pos]) {
    return 0xFFFF;
  }

  if (!BlinkUp) return 0;
  if (!BlinkDown) return 0;
  //if (pos==5) return 0xFFFF; //need to be deleted for testing purpose only!
  int lowBit=blinkMask>>pos;
  lowBit=lowBit&B00000001;
  
  static unsigned long lastTimeEditBlink=nowMillis;
  static bool blinkState=false;
  word mask=0;
  static int tmp=0;//blinkMask;
  if ((nowMillis-lastTimeEditBlink)>300)
  {
    lastTimeEditBlink=nowMillis;
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
  if ((((dotPattern&~tmp)>>6)&1)==1) digitalWrite(pinLowerDots, HIGH);
      else digitalWrite(pinLowerDots, LOW);
  if ((((dotPattern&~tmp)>>7)&1)==1) digitalWrite(pinUpperDots, HIGH);
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
  if (value[xlateMenuPosition(DateYearIndex)]%4==0) days[1]=29;
  if (value[xlateMenuPosition(DateDayIndex)]>days[value[xlateMenuPosition(DateMonthIndex)]-1]) return false;
    else return true;
  
}

byte default_dur = 4;
byte default_oct = 6;
int bpm = 63;
int num;
long wholenote;
unsigned long duration;
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
#ifdef INCLUDE_TONES
unsigned long stopPlaying=0;

void enableTimer() {
  TCCR2A = 0;
  TCCR2B = 0;
  bitWrite(TCCR2A, WGM21, 1);
  bitWrite(TCCR2B, CS20, 1);
}
/*
TCCRx - Timer/Counter Control Register. The prescaler can be configured here.
TCNTx - Timer/Counter Register. The actual timer value is stored here.
OCRx - Output Compare Register
ICRx - Input Capture Register (only for 16bit timer)
TIMSKx - Timer/Counter Interrupt Mask Register. To enable/disable timer interrupts.
TIFRx - Timer/Counter Interrupt Flag Register. Indicates a pending timer interrupt.
 */

void disableTimer()
{
  if (nowMillis < stopPlaying || stopPlaying == 0) {
    return;
  }

  stopPlaying = 0;
#if defined(TIMSK2) && defined(OCIE2A)
  bitWrite(TIMSK2, OCIE2A, 0); // disable interrupt
#endif

  // configure timer 2 for phase correct pwm (8-bit)
#if defined(TCCR2A) && defined(WGM20)
  TCCR2A = (1 << WGM20);
#endif

  // set timer 2 prescale factor to 64
#if defined(TCCR2B) && defined(CS22)
  TCCR2B = (TCCR2B & 0b11111000) | (1 << CS22);
#endif

#if defined(OCR2A)
  OCR2A = 0;
#endif
}

void playTone(uint16_t freq, uint32_t dur) {
  stopPlaying = nowMillis + dur;
  enableTimer();
  tone1.play(freq, dur);
}

 // now begin note loop
 static unsigned long lastTimeNotePlaying=0;
 char* playmusic(char *p)
  {
     if(p == 0 || *p==0)
      {
        return p;
      }
     if (stopPlaying == 0) {
       enableTimer();
     }
    if (nowMillis-lastTimeNotePlaying>duration) {
        lastTimeNotePlaying=nowMillis;
    } else {
      return p;
    }
    // first, get note duration, if available
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    
    if(num) duration = wholenote / num;
    else duration = wholenote / default_dur;  // we will need to check if we are a dotted note after

    stopPlaying = nowMillis + duration;

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
      if (nowMillis-lastTimeNotePlaying>duration)
        lastTimeNotePlaying=nowMillis;
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
#endif
byte xlateMenuPosition(byte inPos) {
  if (DateYearIndex >= inPos && inPos >= DateDayIndex) {
    return dateIndexLookup[value[DateFormatIndex]][inPos-DateDayIndex];
  } else {
    return inPos;
  }
}

void incrementValue() {
  enteringEditModeTime = nowMillis;
  if (editMode == true) {
    byte xlateMenuPos = xlateMenuPosition(menuPosition);

    if (menuPosition != hModeValueIndex) // 12/24 hour mode menu position
      value[menuPosition] = value[menuPosition] + 1;
    else
      value[menuPosition] = value[menuPosition] + 12;

    if (value[menuPosition] > maxValue[xlateMenuPos])
      value[menuPosition] = minValue[xlateMenuPos];

    if (menuPosition == AlarmArmedIndex) {
      if (value[menuPosition] == 1) /*digitalWrite(pinUpperDots, HIGH);*/
        dotPattern = B10000000; //turn on all dots
      /*else digitalWrite(pinUpperDots, LOW); */dotPattern = B00000000; //turn off all dots
    }

    injectDigits(blinkMask, value[menuPosition]);
  }
}

void decrementValue() {
  enteringEditModeTime = nowMillis;
  if (editMode == true) {
    byte xlateMenuPos = xlateMenuPosition(menuPosition);

    if (menuPosition != hModeValueIndex)
      value[menuPosition] = value[menuPosition] - 1;
    else
      value[menuPosition] = value[menuPosition] - 12;

    if (value[menuPosition] < minValue[xlateMenuPos])
      value[menuPosition] = maxValue[xlateMenuPos];

    if (menuPosition == AlarmArmedIndex) {
      if (value[menuPosition] == 1) /*digitalWrite(pinUpperDots, HIGH);*/
        dotPattern = B10000000; //turn on upper dots включаем верхние точки
      else
        /*digitalWrite(pinUpperDots, LOW);*/dotPattern = B00000000; //turn off upper dots
    }

    injectDigits(blinkMask, value[menuPosition]);
  }
}

bool Alarm1SecondBlock=false;
unsigned long lastTimeAlarmTriggired=0;
void checkAlarmTime()
{
 if (value[AlarmArmedIndex]==0) return;
 if ((Alarm1SecondBlock==true) && ((nowMillis-lastTimeAlarmTriggired)>1000)) Alarm1SecondBlock=false;
 if (Alarm1SecondBlock==true) return;
 if ((GLOB_TIME[HOUR_IDX]==value[AlarmHourIndex]) && (GLOB_TIME[MIN_IDX]==value[AlarmMinuteIndex]) && (GLOB_TIME[SEC_IDX]==value[AlarmSecondIndex]))
   {
     lastTimeAlarmTriggired=nowMillis;
     Alarm1SecondBlock=true;
     Serial.println(F("Wake up, Neo!"));
#ifdef INCLUDE_TONES
     p=song;
#endif
   }
}

unsigned long displayOverrideStart = 0;

boolean checkDisplay(boolean override) {
	if (override) {
		displayOverrideStart = nowMillis;
	}

	// If we should be displaying digits or alarm is playing turn HV on. Otherwise turn off
	byte hour = GLOB_TIME[HOUR_IDX];
	boolean displayDigits = hour >= value[DigitsOnIndex] && hour < value[DigitsOffIndex];
	if (value[DigitsOnIndex] >= value[DigitsOffIndex]) {
	  displayDigits = hour >= value[DigitsOnIndex] || hour < value[DigitsOffIndex];
	}
  if (((nowMillis - displayOverrideStart) <= 5000) || displayDigits
#ifdef INCLUDE_TONES
      || (p != 0 && *p != 0)
#endif
    )
  {
	  digitalWrite(DHVpin, HIGH);
	  return true;
  } else {
	  digitalWrite(DHVpin, LOW); // off MAX1771 Driver  High Voltage(DHV) 110-220V
	  return false;
  }
}

void setHueFromEEPROM()
{
  hue = EEPROM.read(HueEEPROMAddress);
}

#ifdef PLOT_LEDS
void plotLEDs() {
  int16_t comPkt[6];   //Buffer to hold the packet, note it is 16bit data type

  comPkt[0] = 0xCDAB;               //Packet Header, indicating start of packet.
  comPkt[1] = 4 * sizeof(int16_t);  //Size of data payload in bytes.
  comPkt[2] = (int16_t) RedLight;
  comPkt[3] = (int16_t) GreenLight;
  comPkt[4] = (int16_t) BlueLight;
  comPkt[5] = (int16_t) hue;

   Serial.write((uint8_t *)comPkt, 12);
   Serial.println(' ');
}
#endif

void printLEDs(const char str[]) {
  Serial.print(str);
  Serial.print(" LEDs: ");
  Serial.print(RedLight);
  Serial.print(',');
  Serial.print(GreenLight);
  Serial.print(',');
  Serial.println(BlueLight);
}
