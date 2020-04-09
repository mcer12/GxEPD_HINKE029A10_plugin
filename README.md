# GxEPD HINK-E029A10 arduino plugin
This small Arduino library adds support for **2.9" HINK-E029A10-A1** display to GxEPD library. It is a 3-color display with SSD1675A driver.

This plugin doesn't support partial update atm, other than that it should work. PRs are welcome.

https://github.com/ZinggJM/GxEPD

## Installation
1. copy the repository to your Arduino/libraries folder
2. add "#include <HINKE029A10.h>" after "#include <GxEPD.h>"
3. And done. You can try it out with example attached in the repository.

## Notes
On ESP8266 I suggest to map CS pin to GPIO5 / D1 in order for the device to boot up correctly

## Based mostly on these projects:
- https://github.com/drewler/epd
- https://github.com/atc1441/GxEPD
