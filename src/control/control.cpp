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


// FLYING GUIDE

// 1. run script ./flyme.sh in top directory
// 2. press enter to start up copter- keep leap clear of hands
// 3. bring one hand (NOTE: spread fingers of dominant hand) over the leap in a low position
// 4. slowly raise the hand to reach the desired altitude

// NORMAL -> HOVER: on the side, flash >2 fingers with your unused hand
// NORMAL -> LAND: remove hand OR make hand into fist

// HOVER -> NORMAL: on the side, flash >2 fingers with your unused hand
// HOVER -> LAND: on the side, flash a fist if your unused hand. QUICKLY make your
//		  dominant hand into a fist as well

// LAND -> NORMAL: show the leap 1 hand

// NORMAL: in NORMAL mode, you are responsible for thrust, pitch, and roll. It is
//	   advised to reach a desired altitude in NORMAl and switch to HOVER
// HOVER: in HOVER mode, you are responsible for pitch, roll, and yaw. You are
//	  advised to keep your in the middle (heigh-wise) in order to not change
//	  yaw unless necessary

#include <cflie/CCrazyflie.h>
#include <stdio.h>
#include <unistd.h>
#include "../leap/leap_c.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>

using namespace std;

// Thread processing
// Use this lock to help synchronize the leap and copter threads
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

//CS50_TODO:  define your own states
//The states of the copter
#define NORMAL_STATE 1
#define HOVER_STATE 2 // Maintain current altitude
#define PRE_NORMAL_STATE 3 // Transition between NORMAL and HOVER
#define PRE_HOVER_STATE 4 // Transition between NORMAL and HOVER
#define LAND_STATE 5 // Reduce thrust gradually

//CS50_TODO:  define your own signals
//These signals are used to indicate to from the leap thread to the coptor
//thread which state to change to
#define NO_SIG 11//Each time when our state machine process the current signal
#define CHANGE_HOVER_SIG 12//Use to transit state between Normal and Hover
#define TIME_OUT_SIG 13
#define LAND_SIG 14 // Used to transition from FLY to LAND
#define RETURN_NORMAL_SIG 15

//trim value in case the coptor is not balanced
//         + P
//	    |
//          |
// + R -----+----- - R
//	    |
//	    |
//         - P
#define TRIM_ROLL 0
#define TRIM_PITCH -.2

// Threshold values that are used when calculating roll and pitch. If the hand
// is within these values, roll and pitch are zero
#define NEG_ROL_THRESHOLD -.1
#define POS_ROL_THRESHOLD .1
#define NEG_PTH_THRESHOLD -.1
#define POS_PTH_THRESHOLD .1

// Threshold value used to for calculating yaw. If the height is outside the threshold
// then we change the yaw.
#define YAW_THRESHOLD 75

// Constant values for roll and pitch that are set when the hand has crossed
// the threshold. Used when we don't have fine-grained hand gestures.
#define NEG_ROLL -8
#define POS_ROLL 8
#define NEG_PTH -8
#define POS_PTH 8

// Values used for fine-grain control. See function for details
#define MAX_ROLL 10
#define MAX_PITCH 10

// the values for roll and pitch usually vary between -10 and 10; this allows
// you to change the value by a factor
#define SENSITIVITY 2

// Used to calculate the factor for the thrust; max and min height we will use
// to calculate thrust. We will warn the user when his hand is not in this range
#define MAX_HEIGHT 500
#define MIN_HEIGHT 60

// Lowest thrust in NORMAL mode
#define MIN_THRUST 30000
// Maximum thrust in NORMAL mode
#define MAX_THRUST 60000

// Lowest value that thrust will reach
#define THRUST_START 10001

// Hover thrust value- coptor will maintain altitude
#define HOVER_THRUST 32767

// Sets how quickly the thrust will be reduced; the value is simply how quickly
// the thrust is reduced each CYCLE of the coptor thread
#define LAND_CONST 25

// Constant inveral for sending out TIME_OUT_SIG
#define TIME_OUT_INTERVAL 1000

//CS50_TODO:  define other variables here
//Such as: current states, current thrust
//variable for state and signals
// Current signal. Defaults to NO_SIG after any signal is processed
int current_signal = NO_SIG;
// Current state; starts at land state
int current_state = LAND_STATE;
// Current pitch value of the copter
float current_pitch;
// Current roll value of the copter
float current_roll;
// Current yaw value of the copter
float current_yaw;
// Current thrust value of the copter
float current_thrust;

// Last time copter state was updated. These two variables are used to determine
// when to send out TIME_OUT_SIG
double lastUpdate;
double currTime;

//The pointer to the crazy flie data structure
CCrazyflie *cflieCopter=NULL;

//CS50_TODO:  define other helper function here
//In normal state, the flyNormal function will be called, set different parameter
//In hover state, different function should be called, because at that time, we should set the thrust as a const value(32767), see reference
//(http://forum.bitcraze.se/viewtopic.php?f=6&t=523&start=20)
//hoverNormal(CCrazyflie *cflieCopter) -> sets thrust to const value(32767)
//We can still control direction when a copter is hovering, but we cannot control thrust -> it will be constant

//The three helper functions that determine the thrust, pitch, roll, and yaw in each of the five states
void hoverNormal(CCrazyflie *cflieCopter) {
  setThrust(cflieCopter, HOVER_THRUST); // Maintain altitude
  setPitch(cflieCopter, current_pitch + TRIM_PITCH);
  setRoll(cflieCopter, current_roll + TRIM_ROLL);
  setYaw(cflieCopter, current_yaw);
}

void flyNormal(CCrazyflie *cflieCopter) {
  setThrust(cflieCopter, current_thrust);
  setPitch(cflieCopter, current_pitch + TRIM_PITCH);
  setRoll(cflieCopter, current_roll + TRIM_ROLL);
  setYaw(cflieCopter, 0);
}

// Make the roll and pitch to zero and then slowly subtract the thrust until it
// reaches the starting thrust. It will also quickly bring down the thrust if it
// already set very high
void land(CCrazyflie *cflieCopter) {
  setPitch(cflieCopter, 0);
  setRoll(cflieCopter, 0);
  setYaw(cflieCopter, 0);
  if(current_thrust > ((MIN_THRUST + MAX_THRUST) / 2)){
    current_thrust = ((MIN_THRUST + MAX_THRUST) / 2) - 1;
    setThrust(cflieCopter, current_thrust);
  } else if(current_thrust - LAND_CONST < THRUST_START) {
    setThrust(cflieCopter, THRUST_START); // Defaults to here
  } else {
    current_thrust -= LAND_CONST;
    setThrust(cflieCopter, current_thrust);
  }
}

// Sets the thrust based on the height of the hand. The thrust will vary between
// MIN_THRUST and MAX_THRUST depending on the height of the hand. If the
// hand is too height then the thrust is set to MAX_THRUST and
// if the hand is too low then the thrust is set to MIN_THRUST
// NOTE: if the hand goes out of frame, the coptor will still go into
// LAND mode. The min and max heights allow some warning for the user if his hand
// is about to leave the recording area.
void updateThrust(double height){
  //for standard, battery regulated thrust
  //double factor = 52 - (4 * batteryLevel(cflieCopter));
  //current_thrust = 38500 + height * factor;

  //variable thrust as described above
  if ((height - MIN_HEIGHT) > MAX_HEIGHT) {// too high
    current_thrust = MAX_THRUST;
    printf("WARNING: lost hand- too HIGH!\n");
  } else if (height < MIN_HEIGHT) {// too low
    current_thrust = MIN_THRUST;
    printf("WARNING: lost hand- too LOW!\n");
  } else {// just right
    double thrust = ((height - MIN_HEIGHT) / MAX_HEIGHT);
    current_thrust = thrust * (MAX_THRUST - MIN_THRUST) + MIN_THRUST;
  }
}

// Sets the yaw depending on the height of the hand. If the height
//is inside the threshold then the yaw is set.
// NOTE: yaw is only changed when in HOVER mode
void updateYaw(double height){
  if (height > MAX_HEIGHT - YAW_THRESHOLD){
    current_yaw = 20 * SENSITIVITY;
  } else if (height < MIN_HEIGHT + YAW_THRESHOLD){
    current_yaw = -20 * SENSITIVITY;
  } else {
    current_yaw = 0;
  }
}

//The leap motion call back functions
//Leap motion functions
void on_init(leap_controller_ref controller, void *user_info)
{
  printf("init\n");
}

void on_connect(leap_controller_ref controller, void *user_info)
{
  printf("connect\n");
}

void on_disconnect(leap_controller_ref controller, void *user_info)
{
  printf("disconnect\n");
}

void on_exit(leap_controller_ref controller, void *user_info)
{
  printf("exit\n");
}

//This function will be called when leapmotion detects a hand gesture
// The function has three main components, each checking for a specific
// number of hands. Depending on the number of hands and the fingers,
// the current_signal will be set. The function also takes in information
// about the hand and updates the flight parameters.
void on_frame(leap_controller_ref controller, void *user_info)
{
  // Lock global variables
  pthread_mutex_lock(&mutex1);
  leap_frame_ref frame = leap_controller_copy_frame(controller, 0);

  // SET SIGNAL

  // Let the coptor process the signal before setting a new signal
  if(current_signal != NO_SIG) {
    pthread_mutex_unlock(&mutex1);
    return;
  }

  // Get number of hands in frame
  int hands = leap_frame_hands_count(frame);

  // no hands detected; send time out if needed and return
  if(hands == 0) {
    currTime = currentTime();
    if(currTime - lastUpdate > TIME_OUT_INTERVAL && current_signal == NO_SIG) {
      if(current_state == PRE_NORMAL_STATE || current_state == PRE_HOVER_STATE) {
	current_signal = TIME_OUT_SIG;
	lastUpdate = currTime;
      }
    }
    //no hand detected so change from NORMAL to LAND
    if (current_state == NORMAL_STATE){
      current_signal = LAND_SIG;
    }
    pthread_mutex_unlock(&mutex1);
    return;
  }

  // Gestures to change states. Leap must see at least one hand
  // Get the current hand
  leap_hand_ref hand = leap_frame_hand_at_index(frame, 0);

  // If hand is invalid, skip to next iteraiton of loop
  if(!leap_hand_is_valid(hand)) {
    pthread_mutex_unlock(&mutex1);
    return;
  }

  // Get number of fingers
  int fingers = leap_hand_fingers_count(hand);

  // One hand gestures:
  // fingers spread -> change state from LAND to NORMAL
  // 1 hand fist -> change from NORMAL to LAND
  if (hands == 1){
    if (current_state == LAND_STATE && fingers >= 4) {
      current_signal = RETURN_NORMAL_SIG;
      pthread_mutex_unlock(&mutex1);
      return;
    } else if(current_state == NORMAL_STATE && fingers <= 1){
      current_signal = LAND_SIG;
      pthread_mutex_unlock(&mutex1);
      return;
    }
  }

  // Two hand gestures:
  // 1 hand + 2 fingers -> HOVER or NORMAL to a TRANSITION state
  // 1 hand + fist ->  changes from HOVER to LAND
  if (hands == 2) {
    // Make hand and fingers refer to the second hand
    hand = leap_frame_hand_at_index(frame, 1);
    fingers = leap_hand_fingers_count(hand);
    if (!leap_hand_is_valid(hand)){
      pthread_mutex_unlock(&mutex1);
      return;
    }  // Change state from NORMAL or HOVER to a TRANSITION phase
    if((current_state == NORMAL_STATE || current_state == HOVER_STATE) && fingers >= 2){
      current_signal = CHANGE_HOVER_SIG;
      pthread_mutex_unlock(&mutex1);
      return;
    }
    // Change HOVER to LAND with fist
    else if (current_state == HOVER_STATE && fingers == 0){
      current_signal = LAND_SIG;
      pthread_mutex_unlock(&mutex1);
      return;
    }
    // Make hand and fingers refer back to the first hand
    hand = leap_frame_hand_at_index(frame, 0);
    fingers = leap_hand_fingers_count(hand);
  }

  // Send out TIME_OUT_SIG if the state is TRANSITION phase and a sufficient
  // amount of time has passed
  currTime = currentTime();
  if(currTime - lastUpdate > TIME_OUT_INTERVAL && current_signal == NO_SIG) {
    if(current_state == PRE_NORMAL_STATE || current_state == PRE_HOVER_STATE) {
      current_signal = TIME_OUT_SIG;
      lastUpdate = currTime;
      pthread_mutex_unlock(&mutex1);
      return;
    }
  }

  //  SET THRUST

  // Get position of hand
  leap_vector pos;
  leap_hand_palm_position(hand, &pos);

  // Set thrust for copter based on hand position; adjust if in NORMAL/PRE_NORMAL states
  if(current_state == NORMAL_STATE || current_state == PRE_NORMAL_STATE) {
    updateThrust(pos.y);
  } //if in HOVER/PREHOVER states, adjust the yaw
  else if (current_state != LAND_STATE){
    updateYaw(pos.y);
  }

  // SET PITCH AND ROLL

  // Get direction of hand
  leap_vector dir;
  leap_hand_direction(hand, &dir);

  // Only adjust roll and pitch if not in LAND state and we see 5 fingers
  // For fine-grain control: dir.x and dir.y return values between 1 and -1. If
  // those values are outside of our threshold, then we multiple the value with
  // the maximum allowed pitch or roll values. This gives us the ability to have
  // a range of values from 0 to our max value * (1 - POS_ROL_THRESHOLD). We then
  // multiply this value by some sensitivty which allows for greater adjustment.
  if(current_state != LAND_STATE) {
    if (fingers > 3){//enough fingers to warrant a change in pitch and roll
      if (dir.x < POS_ROL_THRESHOLD && dir.x > NEG_ROL_THRESHOLD) {//within threshold
	current_roll = 0;
      }
      else{// past threshold
	if (dir.x > POS_ROL_THRESHOLD){
	  current_roll = (MAX_ROLL * (dir.x - POS_ROL_THRESHOLD))  * SENSITIVITY;
	} else {
	  current_roll = (MAX_ROLL * (dir.x - NEG_ROL_THRESHOLD))  * SENSITIVITY;
	}
      }

      // Get pitch- note that the pitches are opposite the y values
      if (dir.y < POS_PTH_THRESHOLD && dir.y > NEG_PTH_THRESHOLD) {//within threshold
	current_pitch = 0;
      }
      else{// past threshold
	if (dir.y > POS_PTH_THRESHOLD){
	  current_pitch = (MAX_PITCH * (dir.y - POS_PTH_THRESHOLD) * -1) * SENSITIVITY;
	} else{
	  current_pitch = (MAX_PITCH * (dir.y - NEG_PTH_THRESHOLD) * -1) * SENSITIVITY;
	}
      }
    } else { // there aren't enough fingers detected for an accurate reading so set all to 0
      current_roll = 0;
      current_pitch = 0;
    }
  }

  //CS50_TODO
  //*pseudocode*
  /*
     The above code show a simple example on get the velocity infomarion of the hand

     You need to find headers in the Leap folder, see what function you can call to
     examine the hand's parameter

     void leap_hand_direction(leap_hand_ref hand, leap_vector *out_result)
     - Use leap vector to get direction of hand
     - Use vector to set pitch and roll

     void leap_hand_palm_position(leap_hand_ref hand, leap_vector *out_result)
     - Use leap vector to get position of hand in relation to the Leap device
     - Use vector to set thrust

     My personal solution use the orientation of hand to compuate the get pitch and roll
     use the position of hand to get the thrust
     Use a "swipe" gesture to enable/disable the hover mode

     you should also send out the signals in this function:

     velocity = leap vector set by leap_hand_palm_velociy
     if velocity.x > some value
     make sure current_signl is set to NO_SIG first
     set current state to hover -> set current_signal variable to CHANGE_HOVER_SIG
     end

     if direction.x > some value
     set the pitch with some value -> set current_pitch variable to dir.pitch
     set the roll with some value -> set current_roll variable to dir.roll
     end

     if pos.y > some value
     set the current_thrust variable to pos.y
     end

     This function send out signal by set the current_signal variable

     This function should only send one signal each time, and only send signals when the last signal has been processed

     Recall that after the commander thread will set the current_signal back to NO_SIG, so we will only send out signal
     if we find this variable is equal to NO_SIG

     You may use lock to lock the global variable you use to synchronize the two thread
   */
  //*pseudocode*
  leap_frame_release(frame);
  // Unlock global variables
  pthread_mutex_unlock(&mutex1);
}

//This the leap motion control callback function
//You don't have to modifiy this
void* leap_thread(void * param){
  struct leap_controller_callbacks callbacks;
  callbacks.on_init = on_init;
  callbacks.on_connect = on_connect;
  callbacks.on_disconnect = on_disconnect;
  callbacks.on_exit = on_exit;
  callbacks.on_frame = on_frame;
  leap_listener_ref listener = leap_listener_new(&callbacks, NULL);
  leap_controller_ref controller = leap_controller_new();
  leap_controller_add_listener(controller, listener);
  while(1);
}

// The thread checks the curren state and signal and sets the correct state.
// It also sends the flight parameters to the copter.
void* main_control(void * param){
  CCrazyflie *cflieCopter=(CCrazyflie *)param;

  while(cycle(cflieCopter)) {
    //transition depend on the current state
    // Lock global variables
    pthread_mutex_lock(&mutex1);
    if(current_signal != NO_SIG) {
      switch(current_signal) {
	case RETURN_NORMAL_SIG:
	  currTime = currentTime();
	  if (currTime - lastUpdate > TIME_OUT_INTERVAL) {
	    current_state = NORMAL_STATE;
	    printf("STATE: NORMAL\n\n");
	    lastUpdate = currTime;
	  }
	  break;
	case TIME_OUT_SIG:
	  if(current_state == PRE_NORMAL_STATE) {
	    current_state = NORMAL_STATE;
	    printf("STATE: NORMAL\n\n");
	  } else if(current_state == PRE_HOVER_STATE) {
	    current_state = HOVER_STATE;
	    printf("STATE: HOVER\n\n");
	  }
	  break;
	case CHANGE_HOVER_SIG:
	  currTime = currentTime();
	  if (currTime - lastUpdate > TIME_OUT_INTERVAL) {
	    if(current_state == NORMAL_STATE) {
	      current_state = PRE_HOVER_STATE;
	      printf("Switching to HOVER...\n");
	      lastUpdate = currTime;
	    } else if(current_state == HOVER_STATE) {
	      current_state = PRE_NORMAL_STATE;
	      printf("Switching to NORMAL...\n");
	      lastUpdate = currTime;
	    }
	  }
	  break;
	case LAND_SIG:
	  current_state = LAND_STATE;
	  printf("STATE: LAND\n\n");
	  break;
	default:
	  break;
      }
    }

    // Reset signal because we processed it
    current_signal = NO_SIG;

    // Call helper functions based on the state to set thrust, pitch, and roll
    // When a coptor is in a TRANSITON state, it behaves as if it is already
    // in the next state. This makes it feel as if the coptor goes from one state
    // directly into the next, even though there is a time out period to prevent
    // for repeatedly switching states
    if(current_state == NORMAL_STATE || current_state == PRE_NORMAL_STATE) {
      flyNormal(cflieCopter);
      cflieCopter->m_setHoverPoint = -2;
    }
    else if(current_state == HOVER_STATE || current_state == PRE_HOVER_STATE) {
      // Change m_setHoverPoint to tell copter that it should hover
      hoverNormal(cflieCopter);
      cflieCopter->m_setHoverPoint = 2;
    }
    else if(current_state == LAND_STATE) {
      land(cflieCopter);
      cflieCopter->m_setHoverPoint = -2;
    }
    // Unlock global variables
    pthread_mutex_unlock(&mutex1);

  }
  //CS50_TODO : depend on the current signal and current state, you can call the helper function here to control the copter
  //switch(current_signal){
  //  Once a state has been changed print out what the state has been changed to
  //}
  // After processing switch statement set current_signal to NO_SIG
  printf("%s\n", "exit");
  return 0;
}

//This this the main function, use to set up the radio and init the copter
int main(int argc, char **argv) {
  CCrazyRadio *crRadio = new CCrazyRadio;
  //CS50_TODO
  //The second number is channel ID
  //The default channel ID is 10
  //Each group will have a unique ID in the demo day
  CCrazyRadioConstructor(crRadio,"radio://0/38/250K");

  if(startRadio(crRadio)) {
    cflieCopter = new CCrazyflie;
    CCrazyflieConstructor(crRadio,cflieCopter);

    //Initialize the set value
    setThrust(cflieCopter, THRUST_START);

    // Enable sending the setpoints. This can be used to temporarily
    // stop updating the internal controller setpoints and instead
    // sending dummy packets (to keep the connection alive).
    setSendSetpoints(cflieCopter,true);

    currTime = currentTime();
    lastUpdate = currTime;

    //CS50_TODO
    //Do initialization here
    //And set up the threads
    pthread_t copter, leap;
    // Start copter thread
    int iret1 = pthread_create(&copter, NULL, main_control, cflieCopter);
    // Start leap thread
    int iret2 = pthread_create(&leap, NULL, leap_thread, NULL);

    while(1) {}
    exit(0);

  }
  else {
    printf("%s\n", "Could not connect to dongle. Did you plug it in?");
  }
  return 0;
}
