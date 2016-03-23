# AttackOfTheDrones
CS50 Final Project: Hand Gesture-based Nanocopter Control

## Overview

The report on the general design and specifications (finite state machine, data structures) is in `report.pdf`.

The project focused on flying a [Crazyflie 1.0](https://www.bitcraze.io/crazyflie/) nanocopter with a [Leap Motion](https://www.leapmotion.com/) sensor. The sensor recognizes custom gestures to control the nanocopter. We communicate with the nanocopter using a [USB dongle](https://www.bitcraze.io/2012/02/the-crazyradio-dongle/) included with the Crazyflie.

The nanocopters run an embedded OS named RTOS. We focused on writing the code that would allow the motion sensor to communicate with the nanocopter. Some pieces of code were supplied to us.

## Getting Started
Try following the [Crazyflie startup guide](https://github.com/bitcraze/crazyflie-clients-python/blob/master/README.md). This didn't fully work for me, so I document my setup below. Follow at your own risk!

### Crazyflie Dependencies
1. macports (package installer)
  - http://www.macports.org/install.php
  - might need to install xcode
  - use a new terminal window to use macports
1. libusb
  - `sudo port install libusb`
  - might need to install Xcode command line tools
1. pyusb
  - instructions: https://github.com/walac/pyusb
1. pygame
  - `pip install hg+http://bitbucket.org/pygame/pygame`
1. pyQT
  - `sudo port install py34-pyqt4`
1. pyqtgraph
  - download "source package": http://www.pyqtgraph.org/
  - within unzipped directory: `python setup.py install`
1. libusb
  - `brew install libusb`
1. libLeap
  - `sudo cp bin/libLeap.dylib /opt/local/lib ` 

### Compile
Note: there may be system-dependent things that need to be changed.
1. `cd bin`
1. `cmake ..`

### Run
1. run ./flyme.sh
1. stay clear of the Leap
1. press `enter` to start up copter
1. bring one hand with spread fingers about 5cm over the Leap
1. slowly raise your hand and the copter will rise

### State Overview

- NORMAL: in NORMAL mode, you are responsible for thrust, pitch, and roll. It is advised to reach a desired altitude in NORMAl and then switch to HOVER

- HOVER: in HOVER mode, you are responsible for pitch, roll, and yaw. You are advised to keep your hand in the middle (height-wise) in order to not change yaw unless necessary

### Changing States

- LAND -> NORMAL: show the leap 1 hand (your dominant, flying hand)

- NORMAL -> HOVER: show 3 or more fingers with your unused hand
- NORMAL -> LAND: remove hand OR make hand into fist

- HOVER -> NORMAL: show 2 or more fingers with your unused hand
- HOVER -> LAND: flash a fist with your unused hand and then also make your dominant hand into a fist
