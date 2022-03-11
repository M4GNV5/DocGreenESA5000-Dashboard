# DocGreenESA5000-Dashboard

This repository contains some utilities and selfmade documentation of the
DocGreen ESA 5000 sold by Lidl. Lidl also sold the Explorer ESA
1919, which is identical protocol wise, but the front light is connected to the
dashboard instead of the motor controller.
The scooter is similar to the Xiaomi M365 however there are some differences:
- the lights are mounted elsewhere
- the light does not change when braking
- it has no connecivity (no bluetooth, no proprietary app, no activation)
- it has a "Zulassung nach eKFV" making it legal to drive on german roads when
equipped with an insurance license plate
- the internal communication uses the same scheme, but the addresses and
contents are different.

## In this repository

- [docgreen-protocol](docgreen-protocol.md): a list of bus messages and the meaning of some of the bytes
- [MegaSniffer](MegaSniffer/): a small arduino program to sniff the internal bus using an Arduino Mega
- [TinyTuning](TinyTuning/): ATtiny45/85 program for tuning ESA Scooters (bus read and write)
- [TinyTuningButton](TinyTuningButton/): ATtiny45/85 program for tuning ESA Scooters (bus write-only variant)
- [DocGreenDisplay](DocGreenDisplay/): a replacement for the stock head unit using an Arduino Nano or ESP32 and
a 128x32 OLED display.

## TinyTuning(Button)

A small PCB with an ATTiny45 which is connected between dashboard and ECU allowing to set the max speed.

![](https://i.m4gnus.de/2020-11/2020-11-15T14:55.JPG)

## Dashboard PCB

The initial solution was a perfboard + Arduino Nano + diode & resistor solution using
[this circuit](https://i.m4gnus.de/12d7a2.svg). It worked well, but was too large to fit into the dashboard place.
Thus, after using a duct tape solution for a few months i decided to create a custom PCB with the same
size as the original dashboard.

On https://jakobloew.me/scooter one can find the manual for the dashboard, with a detailed description of
all features and functionality.

![](https://i.m4gnus.de/2020-09/2020-09-04T12:42.JPG)

## Further Reading

### Other ESA 5000/1919 Projects
- https://github.com/Bert-Tec/xbot_display
- https://github.com/ands/EsaBusModule
- https://github.com/mxbode/ESA5000SetSpeedButton
- https://github.com/crexodon/ESA5000_Dashboard

### Projects for other Scooters
- https://github.com/augisbud/m365_dashboard
- https://github.com/etransport/ninebot-docs/wiki/protocol M365 protocol documentation.
- https://github.com/camcamfresh/Xiaomi-M365-BLE-Controller-Replacement
- https://scooterhacking.org/

## Pictures

![](https://i.m4gnus.de/2020-09/2020-09-04T12:41.JPG)
![](https://i.m4gnus.de/2020-09/2020-09-04T12:40.PNG)
![](https://i.m4gnus.de/2020-09/2020-09-04T12:52.PNG)

## Photos of Prototypes & Development
![](https://i.m4gnus.de/2020-09/2020-09-04T12:49.jpg)
![](https://i.m4gnus.de/2020-09/2020-09-04T12:50.jpg)
![](https://i.m4gnus.de/2020-09/2020-09-04T12:51.jpg)

## LICENSE

This project is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This project is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this project.  If not, see <https://www.gnu.org/licenses/>.
