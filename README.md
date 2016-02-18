# AttackOfTheDrones

## CS50 Final Project: Hand Gesture-based Nanocopter Control
## Authors: Yondon Fu and Patrick Xu

## Overview

There is a detailed report on the general design, specifications (finite state machine, data structures) can be found in `report.pdf`.

The project focused on programming a [Crazyflie 1.0](https://www.bitcraze.io/crazyflie/) nanocopter and a [Leap Motion](https://www.leapmotion.com/) sensor. By implementing custom gestures, we are able to use the sensor to track hand gestures to control the nanocopter. Signals were sent to the nanocopter using a USB dongle included with the Crazyflie.

The nanocopters run an embedded OS named RTOS. We focused on writing the code that would allow the motion sensor to communicate with the nanocopter. Some pieces of code were supplied to us.

## Start Guide
Install hardware dependencies and use the provided code.

### Crazyflie dependencies
1. macports (package installer)
  - http://www.macports.org/install.php
  - might need to install xcode
  - use a new terminal window to use macports
1. libusb
  - `sudo port install libusb`
  - might need to install xcode command line tools
1. pyusb
  - follow the instructions: https://github.com/walac/pyusb
1. pygame
  - `pip install hg+http://bitbucket.org/pygame/pygame`
1. pyQT
  - `sudo port install py34-pyqt4`
1. pyqtgraph
  - download "source package": http://www.pyqtgraph.org/
  - unzip, go to directory
  - `python setup.py install`

### Quick Startup

1. run ./flyme.sh
1. press `enter` to start up copter- keep clear of the Leap Motion
1. bring one hand with spread fingers about 5cm over the Leap
1. slowly raise your hand and the copter will gain altitude

### State Overview

- NORMAL: in NORMAL mode, you are responsible for thrust, pitch, and roll. It is advised to reach a desired altitude in NORMAl and then switch to HOVER

- HOVER: in HOVER mode, you are responsible for pitch, roll, and yaw. You are advised to keep your in the middle (heigh-wise) in order to not change yaw unless necessary

### Changing States

- NORMAL -> HOVER: on the side, flash 3 or more fingers with your unused hand
- NORMAL -> LAND: remove hand OR make hand into fist

- HOVER -> NORMAL: on the side, flash >2 fingers with your unused hand
- HOVER -> LAND: on the side, flash a fist with your unused hand. QUICKLY make your dominant hand into a fist as well

- LAND -> NORMAL: show the leap 1 hand
