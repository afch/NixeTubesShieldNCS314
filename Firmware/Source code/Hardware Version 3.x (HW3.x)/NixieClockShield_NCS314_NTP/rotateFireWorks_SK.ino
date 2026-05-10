#include <Adafruit_NeoPixel.h>

#define PIN            6
#define NUMPIXELS      8
#define LEDsSpeed      10
const int LEDsDelay=40;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void LEDsSetup()
{
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(50);
}

void LEDsOFF()
{
  for(int i=0;i<NUMPIXELS;i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0)); 
  }
  pixels.show();
}

void LEDsTest()
{
  for(int i=0;i<NUMPIXELS;i++)
  {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0)); 
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(1000);
  for(int i=0;i<NUMPIXELS;i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0)); 
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(1000);
  for(int i=0;i<NUMPIXELS;i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 0, 255)); 
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(1000);
  LEDsOFF();
}

void rotateFireWorks()
{
  if (tone1.isPlaying()) return;
  //int LEDspeed=10;
  if (!RGBLedsOn)
  {
    LEDsOFF();
   // analogWrite(RedLedPin, 0 );
   // analogWrite(GreenLedPin, 0);
   // analogWrite(BlueLedPin, 0);
    return;
  }
  if (LEDsLock) return;
  RedLight = RedLight + LEDsSpeed * fireforks[rotator * 3];
  GreenLight = GreenLight + LEDsSpeed * fireforks[rotator * 3 + 1];
  BlueLight = BlueLight + LEDsSpeed * fireforks[rotator * 3 + 2];

  for(int i=0;i<NUMPIXELS;i++)
  {
    pixels.setPixelColor(i, pixels.Color(RedLight, GreenLight, BlueLight)); 
  }

  pixels.show(); // This sends the updated pixel color to the hardware.
  
  cycle = cycle + 1;
  if (cycle == 255/LEDsSpeed)
  {
    rotator = rotator + 1;
    cycle = 0;
  }
  if (rotator > 5) rotator = 0;
}

void setLEDsFromEEPROM()
{
  int R,G,B;
  R=EEPROM.read(LEDsRedValueEEPROMAddress);
  G=EEPROM.read(LEDsGreenValueEEPROMAddress);
  B=EEPROM.read(LEDsBlueValueEEPROMAddress);

  for(int i=0;i<NUMPIXELS;i++)
  {
    pixels.setPixelColor(i, pixels.Color(R, G, B)); 
  }
  pixels.show();
  /*analogWrite(RedLedPin, EEPROM.read(LEDsRedValueEEPROMAddress));
  analogWrite(GreenLedPin, EEPROM.read(LEDsGreenValueEEPROMAddress));
  analogWrite(BlueLedPin, EEPROM.read(LEDsBlueValueEEPROMAddress));*/
    // ********
  Serial.println(F("Readed from EEPROM"));
  Serial.print(F("RED="));
  Serial.println(R);
  Serial.print(F("GREEN="));
  Serial.println(G);
  Serial.print(F("Blue="));
  Serial.println(B);
  // ********
}
