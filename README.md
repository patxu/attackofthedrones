# AttackOfTheDrones
CS50 Final Project: Hand Gesture-based Nanocopter Control

## Overview

The report on the general design and specifications (finite state machine, data structures) is in `report.pdf`.

The project focused on flying a [Crazyflie 1.0](https://www.bitcraze.io/crazyflie/) nanocopter with a [Leap Motion](https://www.leapmotion.com/) sensor. The sensor recognizes custom gestures to control the nanocopter. We communicate with the nanocopter using a [USB dongle](https://www.bitcraze.io/2012/02/the-crazyradio-dongle/) included with the Crazyflie.

The nanocopters run an embedded OS named RTOS. We focused on writing the code that would allow the motion sensor to communicate with the nanocopter. Some pieces of code were supplied to us.

## Getting Started
Try following the [Crazyflie startup guide](https://github.com/bitcraze/crazyflie-clients-python/blob/master/README.md). This didn't fully work for me, so I document my setup below. Follow at your own risk!

### Installation (using MacPorts)
1. macports (package installer)
  - [Install if needed](http://www.macports.org/install.php)
  - Otherwise, upgrade:
    ```
    sudo port selfupdate
    
    sudo port upgrade outdated
    ```
  - open a new terminal window to use macports
1. install dependencies (this might take a while)
  - `sudo port install libusb python34 py34-SDL2 py34-pyqt4 py34-pip`
1. pyusb
  - `python -m pip install "pyusb>=1.0.0b2"`
1. Possibly optional?
  1. pygame
    - `pip install hg+http://bitbucket.org/pygame/pygame`
  1. pyqtgraph (might not be necessary?)
    - [download "source package"](http://www.pyqtgraph.org/)
    - `python setup.py install`
1. move libLeap to correct location (better way to do this?)
  - `sudo cp bin/libLeap.dylib /opt/local/lib `

### Compiling the Crazyflie Client
This is the Crazyflie client and will determine the channel ID of the quadcopter. See [here](https://github.com/bitcraze/crazyflie-clients-python) if you're unable to compile.

1. `cd crazyflie-clients-python`
1. `pip install -e .`
1. `/opt/local/bin/python3.4 bin/cfclient`
1. Select `Select an interface` to determine the channel ID
  - radio://0/`[Channel ID]`/250k
1. Set this channel ID in `control.cpp` line 589

### Compiling our Client
1. `cd bin`
1. `cmake ..`

### Run
1. `./flyme.sh`
1. stay clear of the Leap
1. press `enter` to start up copter
1. bring one hand with spread fingers about 5cm over the Leap
1. slowly raise your hand and the copter will rise

### State Overview

- NORMAL: in NORMAL mode, you are responsible for thrust (height), pitch (forwards and back), and roll. It is advised to reach a desired altitude in NORMAL and then switch to HOVER

- HOVER: in HOVER mode, the copter holds a steady height and you are responsible for pitch, roll, and yaw. Keep your hand at a steady height in order to not change yaw unless necessary

### Changing States

- LAND -> NORMAL: show the leap 1 hand (your dominant, flying hand)

- NORMAL -> HOVER: show 3 or more fingers with your unused hand
- NORMAL -> LAND: remove hand OR make hand into fist

- HOVER -> NORMAL: show 2 or more fingers with your unused hand
- HOVER -> LAND: flash a fist with your unused hand and then also make your dominant hand into a fist

Note: the propeller between the red and green lights is "forward". See more [here](https://wiki.bitcraze.io/projects:crazyflie:userguide:index).
