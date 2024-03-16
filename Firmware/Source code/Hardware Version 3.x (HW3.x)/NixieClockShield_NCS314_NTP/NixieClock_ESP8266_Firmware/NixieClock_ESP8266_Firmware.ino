/****************************************************************************************************
 * BuildXYZ NixieClock ESP8266 NTP provider
****************************************************************************************************/

#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

#define SSID            "NixieClock"
#define PASSWORD        "changeme"
#define PORTAL_TIMEOUT  600           // 600s

#define NTP_SERVER      "pool.ntp.org"
#define NTP_TIMEOUT     3600000       // 3600000 -> 1 hour without NTP response before reboot
#define NTP_INTERVAL    1000 * 60         // 60000mS -> 60 seconds betwteen NTP queries
#define BOOT_DELAY      10000

uint32_t getTime();
void sendNTPpacket(IPAddress& address);

// Globals
WiFiUDP UDP;
WiFiManager wifiManager;
IPAddress timeServerIP;
const int NTP_PACKET_SIZE = 48;
byte NTPBuffer[NTP_PACKET_SIZE];
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
uint32_t timeUNIX = 0;
unsigned long prevActualTime = 0;
unsigned long currentMillis = 0;
uint32_t actualTime = 0;

void setup() {
  Serial.begin(9600);

  // Allow for NixieClock to boot so messages can be viewed
  delay(BOOT_DELAY);
  
  wifiManager.setDebugOutput(false);
  
  // Configuration portal stays up for this amount of time on powerup
  wifiManager.setConfigPortalTimeout(PORTAL_TIMEOUT);

  //exit after config instead of connecting
  wifiManager.setBreakAfterConfig(true);

  pinMode(2, INPUT_PULLUP);
  
  //tries to connect to last known settings
  //if it does not connect it starts an access point and goes into a blocking loop awaiting configuration
  Serial.println("S Connecting to AP");
  if (!wifiManager.autoConnect(SSID, PASSWORD)) {
    Serial.println("E AP Error Resetting ESP8266");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  UDP.begin(123);
  
  if(!WiFi.hostByName(NTP_SERVER, timeServerIP)) { // Get the IP address of the NTP server
    Serial.println("E DNS Error");
    Serial.flush();
    ESP.reset();
  }
  sendNTPpacket(timeServerIP); 
}

void loop() {
  currentMillis = millis();
  
  if (currentMillis - prevNTP > NTP_INTERVAL) { // If a NTP_INTERVAL has passed since last NTP request
    sendNTPpacket(timeServerIP);               // Send an NTP request
    prevNTP = currentMillis;
  }

  uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
  
  if (time) {                                  // If a new timestamp has been received
    timeUNIX = time;
    Serial.print("U ");
    Serial.println(timeUNIX);         // Print Unix time
    Serial.flush();
    lastNTPResponse = currentMillis;
  } 
  else if ((currentMillis - lastNTPResponse) > NTP_TIMEOUT) {
    Serial.println("E Timeout Error");  
    Serial.flush();
    ESP.reset();
  }

  //reset settings if GPIO2 pulled low on boot
  if (!digitalRead(2)) {
    wifiManager.resetSettings();
    ESP.reset();
  }
} // end loop

uint32_t getTime() {
  if (UDP.parsePacket() == 0) { return 0; } // If there's no response (yet)
    
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  
  return UNIXTime;
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

inline int getSeconds(uint32_t UNIXTime) { return UNIXTime % 60; }
inline int getMinutes(uint32_t UNIXTime) { return UNIXTime / 60 % 60; }
inline int getHours(uint32_t UNIXTime) { return UNIXTime / 3600 % 24; }
