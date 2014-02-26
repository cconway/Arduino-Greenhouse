Arduino-Controlled Greenhouse
==================
This project is the open source release of my work on a hobby project to build an Arduino-controlled indoor greenhouse. In the [project description and build log](http://cconway.github.io/Arduino-Greenhouse/) you can learn a lot more about the motivation for the project and how the greenhouse was put together.

In the repo you'll find the Arduino project that operates the greenhouse as well as the XCode project for the iOS app that monitors and controls the greenhouse via Bluetooth LE.

The Arduino project requires a few additional libraries I haven't included in my repo: the Nordic Semiconductor [SDK](http://devzone.nordicsemi.com/arduino) and the [Time](http://www.pjrc.com/teensy/td_libs_Time.html) library.

The XCode project requires a paid iOS Developer account because CoreBluetooth doesn't appear to work, use the Mac's Bluetooth LE hardware, from the simulator and a paid account is required to test the app on real devices.