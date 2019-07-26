//NIXIE COUNTDOWN TIMER FOR NCS314 v2.x SHIELD by GRA & AFCH (fominalec@gmail.com)

//v1.0 26.07.2019
//***********************************
//Mode button - Start/Pause
//Up button - Set to 60 minutes
//Down button - Set to 00:00:00

//IR Remote controller Sony RM-X151:

//"Left" = Up button
//"Enter" = Mode button
//"Right" = Down button
//***********************************
#define UpperDotsMask 0x80000000
#define LowerDotsMask 0x40000000

#include <SPI.h>
#include <Tone.h>
#include <ClickButton.h>
const byte BlueLedPin = 3; 
const byte pinBuzzer = 2;
const byte pinSet = A0;
const byte pinUp = A2;
const byte pinDown = A1;
const int RECV_PIN = 4;
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#include <IRremote.h>
IRrecv irrecv(RECV_PIN);
decode_results IRresults;
// buttons codes for remote controller Sony RM-X151
/*#define IR_BUTTON_UP_CODE 0x6621
#define IR_BUTTON_DOWN_CODE 0x2621
#define IR_BUTTON_MODE_CODE 0x7121

#define IR_BUTTON_SOURCE_CODE 0x3121
#define IR_BUTTON_SCRL_CODE 0x6221
#define IR_BUTTON_OFF_CODE 0x5821
#define IR_BUTTON_ATT_CODE 0x1421
#define IR_BUTTON_MENU_CODE 0x2821
#define IR_BUTTON_SOUND_CODE 0x0421 //421
#define IR_BUTTON_LIST_CAT_CODE 0x7221
#define IR_BUTTON_DSPL_REP_CODE 0x0A21 //A21
*/
 #define IR_BUTTON_LEFT_CODE 0x5621 
 #define IR_BUTTON_ENTER_CODE 0x1D21
 #define IR_BUTTON_RIGHT_CODE 0x1621
/*#define IR_BUTTON_VOLUME_UP_CODE 0x2421 
#define IR_BUTTON_VOLUME_DOWN_CODE 0x6421

#define IR_BUTTON_1_CODE 0x0021 //21
#define IR_BUTTON_2_CODE 0x4021
#define IR_BUTTON_3_CODE 0x2021 
#define IR_BUTTON_4_CODE 0x6021
#define IR_BUTTON_5_CODE 0x1021 
#define IR_BUTTON_6_CODE 0x5021*/

class IRButtonState
{
  public:
    int PAUSE_BETWEEN_PACKETS = 50;
    int PACKETS_QTY_IN_LONG_PRESS = 18;

  private:
    bool Flag = 0;
    byte CNT_packets = 0;
    unsigned long lastPacketTime = 0;
    bool START_TIMER = false;
    int _buttonCode;

  public: IRButtonState::IRButtonState(int buttonCode)
    {
      _buttonCode = buttonCode;
    }

  public: int IRButtonState::checkButtonState(int receivedCode)
    {
      if (((millis() - lastPacketTime) > PAUSE_BETWEEN_PACKETS) && (START_TIMER == true))
      {
        START_TIMER = false;
        if (CNT_packets >= 2) {
          Flag = 0;
          CNT_packets = 0;
          START_TIMER = false;
          return 1;
        }
        else {
          Flag = 0;
          CNT_packets = 0;
          return 0;
        }
      }
      else
      {
        if (receivedCode == _buttonCode) {
          Flag = 1;
        }
        else
        {
          if (!(Flag == 1)) {
            return 0;
          }
          else
          {
            if (!(receivedCode == 0xFFFFFFFF)) {
              return 0;
            }
          }
        }
        CNT_packets++;
        lastPacketTime = millis();
        START_TIMER = true;
        if (CNT_packets >= PACKETS_QTY_IN_LONG_PRESS) {
          Flag = 0;
          CNT_packets = 0;
          START_TIMER = false;
          return -1;
        }
        else {
          return 0;
        }
      }
    }
};

IRButtonState IRLeftButton(IR_BUTTON_LEFT_CODE);
IRButtonState IREnterButton(IR_BUTTON_ENTER_CODE);
IRButtonState IRRightButton(IR_BUTTON_RIGHT_CODE);
#endif

int LeftButtonState = 0;
int EnterButtonState = 0;
int RightButtonState = 0;

const byte LEpin=10; //pin Latch Enabled data accepted while HI level

String stringToDisplay="000000";// Conten of this string will be displayed on tubes (must be 6 chars length)

//                            0      1      2      3      4      5      6      7      8       9
unsigned int SymbolArray[10]={1,     2,     4,     8,     16,    32,    64,    128,   256,    512};

Tone tone1;

//buttons pins declarations
ClickButton setButton(pinSet, LOW, CLICKBTN_PULLUP);
ClickButton upButton(pinUp, LOW, CLICKBTN_PULLUP);
ClickButton downButton(pinDown, LOW, CLICKBTN_PULLUP);
///////////////////

/*******************************************************************************************************
Init Programm
*******************************************************************************************************/
void setup() 
{
  Serial.begin(115200);
  Serial.println(F("Starting"));
  
  pinMode(LEpin, OUTPUT);
  pinMode(pinSet,  INPUT_PULLUP);
  pinMode(pinUp,  INPUT_PULLUP);
  pinMode(pinDown,  INPUT_PULLUP);
  pinMode(pinBuzzer, OUTPUT);

 // SPI setup
  SPI.begin(); // 
  SPI.setDataMode (SPI_MODE2); // Mode 2 SPI
  SPI.setClockDivider(SPI_CLOCK_DIV8); // SCK = 16MHz/8= 2MHz
  stringToDisplay="006000";
  doIndication();
  //pinMode(BlueLedPin, OUTPUT);
  analogWrite(BlueLedPin, 50);
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  irrecv.blink13(false);
  irrecv.enableIRIn(); // Start the receiver
  #endif
  tone1.begin(pinBuzzer);
  tone1.play(1000, 100);

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
}

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
int counter=3600;
bool isStop=false;
unsigned long prevTickTime=millis();
int minutes, seconds;
bool LD,UD;

void loop() 
{
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  IRresults.value = 0;
  if (irrecv.decode(&IRresults)) {
    Serial.println(IRresults.value, HEX);
    irrecv.resume(); // Receive the next value
  }

 LeftButtonState = IRLeftButton.checkButtonState(IRresults.value);
 EnterButtonState = IREnterButton.checkButtonState(IRresults.value);
 RightButtonState = IRRightButton.checkButtonState(IRresults.value);
 #else
 LeftButtonState = 0;
 EnterButtonState = 0;
 RightButtonState = 0;
 #endif

 upButton.Update();
 setButton.Update();
 downButton.Update();

 if ((upButton.clicks > 0) || (LeftButtonState == 1))
 {
  counter=3600;
  updateTubes();
  tone1.play(1000, 100);
 }

 if ((setButton.clicks > 0) || (EnterButtonState == 1)) 
 {
  isStop=!isStop;
  updateTubes();
  tone1.play(1000, 100);
 }

 if ((downButton.clicks > 0) || (RightButtonState == 1))
 {
  counter=0;
  updateTubes();
  tone1.play(1000, 100);
 }
  
 if (!isStop) doDotBlink();
 unsigned long timePassed=millis()-prevTickTime;
 if ((timePassed>1000) && (!isStop))
 {
  prevTickTime=millis();
  //Serial.println("tick");
  counter=counter-1;
  if (counter<=0) 
  {
    counter=0;
    isStop=true;
    Serial.println(F("Stop:00000"));
    tone1.play(400, 500);
  }
  updateTubes();
 }
}

void updateTubes()
{
  minutes=counter/60;
  //Serial.print("min: ");
  //Serial.println(PreZero(minutes));
  seconds=counter%60;
  //Serial.print("sec: ");
  //Serial.println(PreZero(seconds));
  stringToDisplay="00"+PreZero(minutes)+PreZero(seconds);
  doIndication();
}

void doDotBlink()
{
  static unsigned long lastTimeBlink = millis();
  static bool dotState = 0;
  if ((millis() - lastTimeBlink) > 500)
  {
    lastTimeBlink = millis();
    dotState = !dotState;
    if (dotState)
    {
      //dotPattern = B11000000;
      LD=true;
      UD=true;
    }
    else
    {
      //dotPattern = B00000000;
      LD=false;
      UD=false;
    }
    updateTubes();
  }
}

String PreZero(int digit)
{
  digit = abs(digit);
  if (digit < 10) return String("0") + String(digit);
  else return String(digit);
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

  if (LD) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;
  
  if (UD) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;

  SPI.transfer(Var32>>24);
  SPI.transfer(Var32>>16);
  SPI.transfer(Var32>>8);
  SPI.transfer(Var32);

 //-------------------------------------------------------------------------

 //-------- REG 0 ----------------------------------------------- 
  Var32=0;
  
  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // m1
  digits=digits/10;

  Var32|= (unsigned long)(SymbolArray[digits%10])<<10; //h2
  digits=digits/10;

  Var32|= (unsigned long)SymbolArray[digits%10]; //h1
  digits=digits/10;

  if (LD) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;
  
  if (UD) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;

  SPI.transfer(Var32>>24);
  SPI.transfer(Var32>>16);
  SPI.transfer(Var32>>8);
  SPI.transfer(Var32);

  digitalWrite(LEpin, HIGH);     // latching data 
}
