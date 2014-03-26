// Copyright (c) 2013, Jan Winkler <winkler@cs.uni-bremen.de>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Universität Bremen nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <cflie/CCrazyflie.h>
#include <stdio.h>
#include <unistd.h>
#include "../leap/leap_c.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

using namespace std;

// Thread processing
// Use this lock to help synchronize the keyboard and copter threads
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

// States of the copter
#define NORMAL_STATE 1
#define HOVER_STATE 2 // Maintain current altitude
#define LAND_STATE 5 // Reduce thrust gradually

// Signal to tell if a thread has processed a command or not
#define PROCESSED 16
#define NEED_PROCESS 17

//trim value in case the coptor is not balanced
//         + P
//      |
//          |
// + R -----+----- - R
//      |
//      |
//         - P
#define TRIM_ROLL 0
#define TRIM_PITCH 0

// Constant values for roll and pitch to move copter left, right, forward and backward.
// Assume m1 on copter is faced forwards
#define NEG_ROLL -8
#define POS_ROLL 8
#define NEG_PTH -8
#define POS_PTH 8

// Lowest thrust in NORMAL mode
#define THRUST_CONST 40000
// Maximum thrust in NORMAL mode
#define MAX_THRUST 60000

// Lowest value that thrust will reach
#define THRUST_START 10001

// Hover thrust value - coptor will maintain altitude
#define HOVER_THRUST 32767

// Sets how quickly the thrust will be reduced; the value is simply how quickly
// the thrust is reduced each CYCLE of the coptor thread
#define LAND_CONST 50

// Current signal. Defaults to PROCESSED.
int current_signal = PROCESSED;
// Current state; starts at land state
int current_state = LAND_STATE;
// Current pitch value of the copter
float current_pitch = 0;
// Current roll value of the copter
float current_roll = 0;
// Current thrust value of the copter
float current_thrust;

//The pointer to the crazy flie data structure
CCrazyflie *cflieCopter = NULL;

// Keyboard command variable
char command;


//The three helper functions that determine the thrust, pitch, and roll in each of the five states

/*

*hoverNormal*

Description: Makes the copter hover. Maintains a thrust and altitude, but can still change pitch and roll.

Input: CCrazyflie pointer
Return: None

*Pseudo Code*
(1) Set thrust to HOVER_THRUST constant
(2) Set pitch and roll to current pitch and roll

*/
void hoverNormal(CCrazyflie *cflieCopter) {
  setThrust(cflieCopter, HOVER_THRUST); // Maintain altitude
  setPitch(cflieCopter, current_pitch + TRIM_PITCH);
  setRoll(cflieCopter, current_roll + TRIM_ROLL);
}

/*

*flyNormal*

Description: Makes the copter fly normally. Can change altitude/thrust, pitch and roll.

Input: CCrazyflie pointer
Return: None

*Pseudo Code*
(1) Set thrust, pitch and roll to current thrust, pitch and roll

*/
void flyNormal(CCrazyflie *cflieCopter) {
  setThrust(cflieCopter, current_thrust);
  setPitch(cflieCopter, current_pitch + TRIM_PITCH);
  setRoll(cflieCopter, current_roll + TRIM_ROLL);
}

/*

*land*

Description: Set roll and pitch to zero and slowly subtract thrust until it reaches starting thrust.

Input: CCrazyflie pointer
Return: None

*Pseudo Code*
(1) Set pitch and roll to 0
(2) If current thrust - LAND_CONST is less than the starting thrust then set current thrust to THRUST_START
(3) Else just decrease current thrust by the LAND_CONST

*/

void land(CCrazyflie *cflieCopter) { 
  setPitch(cflieCopter, 0);
  setRoll(cflieCopter, 0);
  if(current_thrust - LAND_CONST < THRUST_START) {
    setThrust(cflieCopter, THRUST_START); // Defaults to here
  } else {
    current_thrust -= LAND_CONST;
    setThrust(cflieCopter, current_thrust);
  }
}

/*

*processCommand*

Description: Process the command and determine what state the copter should be set to.

Input: Command to be processed
Return: None

*Pseudo Code*
(1) Check if the command is one of the coded commands
(2) If it is a invalid command print out an error message
(3) Else check what command it is and set the copter to its appropriate state

*/
void processCommand(char command) {
  if(command == 0) return;
  
  // Only change roll/pitch for direction movement for left, right, forward, backward commands.
  // All other commands should keep the copter direcitonally neutral

  if(command == 's') {
    // Take off
    if(current_state == LAND_STATE || current_state == HOVER_STATE) {
      current_state = NORMAL_STATE;
      current_pitch = 0;
      current_roll = 0;
      // Start thrust off at a set constant. Can increase or decrease from there
      current_thrust = THRUST_CONST;
      printf("START\n");
    }
    else {
      printf("Error: need to be in land state or hover state to do this\n");
    } 
  }
  // Increase thrust
  else if(command == '+') {
    if(current_state == NORMAL_STATE) {
      current_thrust += 20;
      current_pitch = 0;
      current_roll = 0;
      printf("Increased thrust by 20\n");
    }
    else {
      printf("Error: need to be in normal state to do this\n");
    }
  }
  // Decrease thrust
  else if(command == '-') {
    if(current_state == NORMAL_STATE) {
      // If subtracting by 20 results in a thrust less than the THRUST_START constant, just set it to
      // THRUST_START
      if(current_thrust - 20 < THRUST_START) current_thrust = THRUST_START;
      else current_thrust -= 20;
      current_pitch = 0;
      current_roll = 0;
      printf("Decreased thrust by 20\n");
    }
    else {
      printf("Error: need to be in normal state to do this\n");
    }
  }
  // Switch to hover 
  else if(command == 'h') {
    if(current_state == NORMAL_STATE) {
      current_state = HOVER_STATE;
      current_pitch = 0;
      current_roll = 0;
      printf("HOVER\n");
    }
    else {
      printf("Error: need to be in normal state to do this\n");
    }
  }
  // Move forward
  else if(command == 'f') {
    if(current_state == NORMAL_STATE || current_state == HOVER_STATE) {
      current_pitch = POS_PTH;
      current_roll = 0;
      printf("FORWARD\n");
    }
    else {
      printf("Error: need to be in normal state or hover state to do this\n");
    }
  }
  // Move backward
  else if(command == 'b') {
    if(current_state == NORMAL_STATE || current_state == HOVER_STATE) {
      current_pitch = NEG_PTH;
      current_roll = 0;
      printf("BACKWARD\n");
    }
    else {
      printf("Error: need to be in normal state or hover state to do this\n");
    }
  }
  // Move left
  else if(command == 'l') {
    if(current_state == NORMAL_STATE || current_state == HOVER_STATE) {
      current_roll = NEG_ROLL;
      current_pitch = 0;
      printf("LEFT\n");
    }
    else {
      printf("Error: need to be in normal state or hover state to do this\n");
    }
  }

  // Move right
  else if(command == 'r') {
    if(current_state == NORMAL_STATE || current_state == HOVER_STATE) {
      current_roll = POS_ROLL;
      current_pitch = 0;
      printf("RIGHT\n");
    }
    else {
      printf("Error: need to be in normal state or hover state to do this\n");
    }
  }

  // Switch to land state
  else if(command == 'd') {
    // Only switch from normal
    if(current_state == NORMAL_STATE) {
      current_state = LAND_STATE;
      current_pitch = 0;
      current_roll = 0;
      printf("LAND\n");
    }
    else {
      printf("Error: need to be in normal state to do this\n");
    }
  }
  // Print out help menu
  else if(command == 'n') {
    printf("s = switch to normal state\n");
    printf("+ = increase thrust\n");
    printf("- = decrease thrust\n");
    printf("h = switch to hover mode and maintain altitude\n");
    printf("f = move the copter forward\n");
    printf("b = move the copter backward\n");
    printf("l = move the copter left\n");
    printf("r = move the copter right\n");
    printf("d = land the copter\n");
  }
  // Bad command
  else {
    printf("Invalid command. Type \"n\" for a list of commands.\n");
  }
}

// The keyboard control thread
void *keyboard(void *param) {
  while(1) {
    // Lock
    pthread_mutex_lock(&mutex1);
    // Has the last command been processed?
    if(current_signal == PROCESSED) {

      // Read user command
      char input[10000];
      printf("Please type in a command:\n");
      fgets(input, sizeof(input), stdin);
      sscanf(input, "%c", &command);

      // Switch states based upon user command
      processCommand(command);

      // Clear input variable
      memset(input, 0, 10000);
      // Reset command variable
      command = 0;

      // A command now needs to be processed!
      current_signal = NEED_PROCESS;
    }
    // Unlock
    pthread_mutex_unlock(&mutex1);
  }
  return 0;
}

/*

*stopHandler*

Description: Catches a signal and sets current_signal to PROCESSED

Input: A signal
Return: None

*Pseudo Code*
(1) Switches current_signal to PROCESSED

*/
void stopHandler(int signal) {
  // Switch signal to PROCESSED
  current_signal = PROCESSED;
}

// The copter control thread
void* main_control(void * param){
  CCrazyflie *cflieCopter=(CCrazyflie *)param;

  while(cycle(cflieCopter)) {
    // Lock
    pthread_mutex_lock(&mutex1);
    // Does a command need to be processed?
    if(current_signal == NEED_PROCESS) {

      // Send command to copter based upon current state
      if(current_state == NORMAL_STATE) {
        flyNormal(cflieCopter);
        cflieCopter->m_setHoverPoint = -2;
      }
      else if(current_state == HOVER_STATE) {
        // Change m_setHoverPoint to tell copter that it should hover
        hoverNormal(cflieCopter);
        cflieCopter->m_setHoverPoint = 2;
      }
      else if(current_state == LAND_STATE) {
        land(cflieCopter);
        cflieCopter->m_setHoverPoint = -2;
      }
    }
    // Unlock
    pthread_mutex_unlock(&mutex1);
  }

  printf("%s\n", "exit");
  return 0;
}

//This this the main function, use to set up the radio and init the copter
int main(int argc, char **argv) {
  CCrazyRadio *crRadio = new CCrazyRadio;
  
  // Set channel for radio
  CCrazyRadioConstructor(crRadio,"radio://0/11/250K");

  // Start up coptor
  if(startRadio(crRadio)) {
    cflieCopter = new CCrazyflie;
    CCrazyflieConstructor(crRadio,cflieCopter);

    //Initialize the set value
    setThrust(cflieCopter, THRUST_START);

    // Enable sending the setpoints. This can be used to temporarily
    // stop updating the internal controller setpoints and instead
    // sending dummy packets (to keep the connection alive).
    setSendSetpoints(cflieCopter,true);

    pthread_t copter, key;
    // Start copter thread
    int iret1 = pthread_create(&copter, NULL, main_control, cflieCopter);
    // Start keyboard thread
    int iret2 = pthread_create(&key, NULL, keyboard, NULL);

    // Catch all Ctrl-Z signals
    // Ctrl-Z is used to stop current command and allow user to input a new command
    signal(SIGTSTP, stopHandler);

    while(1) {}
    exit(0);

  } 
  else {
    printf("%s\n", "Could not connect to dongle. Did you plug it in?");
  }
  return 0;
}
