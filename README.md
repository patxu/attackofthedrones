#CS50 Final Project: Hand Gesture-based Nanocopter Control
##Authors: Yondon Fu and Patrick Xu

##Overview

There is a detailed report on the general design, specifications (finite state machine, data structures) can be found in report.pdf.

The project focused on programming a Crazyflie nanocopter and a Leap Motion Sensor. By implemented custom gestures, we are able to use the sensor to track hand gestures to control the nanocopter. Signals were sent to the nanocopter using a USB radio device that was included with the Crazyflie.

The nanocopters run an embedded OS named RTOS. My partner and I focused on writing the code that would allow the motion sensor to communicate with the nanocopter. Some pieces of code were supplied to us by the professor. 

## FLYING GUIDE

Note: ensure that all controllers are connected and funcitonal, and nanocopter is setup and charged

###Quick Startup

1. run script ./flyme.sh in top directory
2. press enter to start up nanocopter- keep Leam Motion clear of any hands
3. bring one hand (NOTE: spread fingers of dominant hand) about 5cm over the Leam Motion
4. slowly raise the hand to reach the desired altitude

###States Overview

NORMAL: in NORMAL mode, you are responsible for thrust, pitch, and roll. It is advised to reach a desired altitude in NORMAl and then switch to HOVER

HOVER: in HOVER mode, you are responsible for pitch, roll, and yaw. You are advised to keep your in the middle (heigh-wise) in order to not change yaw unless necessary

###Changing States

NORMAL -> HOVER: on the side, flash 3 or more fingers with your unused hand
NORMAL -> LAND: remove hand OR make hand into fist

HOVER -> NORMAL: on the side, flash >2 fingers with your unused hand
HOVER -> LAND: on the side, flash a fist with your unused hand. QUICKLY make your dominant hand into a fist as well

LAND -> NORMAL: show the leap 1 hand 
