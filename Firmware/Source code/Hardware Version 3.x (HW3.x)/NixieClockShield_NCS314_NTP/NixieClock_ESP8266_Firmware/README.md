# NTP Mod for the NCS314 3.x NixieClock shield from GRA&AFCH and Arduino Mega 2560 WiFi ESP8266

ESP8266 taken without changes (except renaming wrong `ESP8622` into `ESP8266` from [this repository](https://github.com/buildxyz-git/nixiclock_ncs314).

Project was modified, tested and flashed with `Arduino 1.8.19 for Windows`.

## Flash ESP8266

Pins 5, 6 and 7 ON, all other pins off. Board: Generic ESP8266 Module. Flash Size: 4M+1M FS (shouldn't matter much).

## Flash Arduino and run NixiClock

Create a new folder `src` in `NixieClockShield_NCS314_NTP` and put the [TimeZone](https://github.com/JChristensen/Timezone) library into it. Rename the folder `Timezone-x.x.x` into `Timezone`.

Pins 1, 2, 3 and 4 ON, all other pins off. Board: Arduino Mega or Mega 2560.

## Wifi setup

Log into wifi network `NixieClock` using password `changeme` and enter the wifi credentials of your wifi network.

The ESP8266 module will now reboot and log into your wifi network using `pool.ntp.org` as NTP server to get the correct time for your NixieClock.
