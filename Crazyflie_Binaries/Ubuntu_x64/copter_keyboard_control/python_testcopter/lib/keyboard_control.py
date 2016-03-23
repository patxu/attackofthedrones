#!/usr/bin/python
import time
import sys
import tty
import termios
import argparse
import cflib
from cflib.crazyflie import Crazyflie
from cfclient.utils.logconfigreader import LogConfig
from cfclient.utils.logconfigreader import LogVariable
from threading import Thread, Event

# Global key for threads
key = ""

class _GetchUnix:
    """
    Class to get keys from the keyboard without pressing Enter.
    """
    def __init__(self):
        pass
    def __call__(self):
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch



class KeyReaderThread(Thread):
    """
    This class is a basic thread for reading the keyboard continously
    """
    def __init__(self):
        """
        Initialize the thread
        """
        Thread.__init__(self)
 
    def run(self):
        """
        Read the keyboard and store the value in variable key
        """
        global key
        getch = _GetchUnix()
        key = getch()
        while key != "e":
            key = getch()
            #time.sleep(0.1)



class TestFlight:
    def __init__(self):
        """
        Initialize the quadcopter
        """
        self.crazyflie = cflib.crazyflie.Crazyflie()
        print 'Initializing drivers' 
        cflib.crtp.init_drivers()
 
        print 'Searching for available devices'
        available = cflib.crtp.scan_interfaces()

        radio = False
        for i in available:
            # Connect to the first device of the type 'radio'
            if 'radio' in i[0]:
                radio = True
                dev = i[0]
                print 'Connecting to interface with URI [{0}] and name {1}'.format(i[0], i[1])
                self.crazyflie.open_link(dev)
                break

        if not radio:
            print 'No quadcopter detected. Try to connect again.'
            exit(-1)

        # Set up the callback when connected
        self.crazyflie.connectSetupFinished.add_callback(self.connectSetupFinished)


    def connectSetupFinished(self, linkURI):
        """
        Set the loggers
        """
        # Log stabilizer
        self.logStab = LogConfig("Stabalizer", 200)
        self.logStab.add_variable("stabilizer.roll", "float")
        self.logStab.add_variable("stabilizer.pitch", "float")
        self.logStab.add_variable("stabilizer.yaw", "float")
        self.logStab.add_variable("stabilizer.thrust", "uint16_t")
 
        self.crazyflie.log.add_config(self.logStab)
 
        if self.logStab.valid:
            self.logStab.data_received_cb.add_callback(self.print_stab_data)
            self.logStab.start()
        else:
            print 'Could not setup log configuration for stabilizer after connection!'

        # Log barometer
        self.logBaro = LogConfig("Baro", 200)
        self.logBaro.add_variable("baro.aslLong", "float")

        self.crazyflie.log.add_config(self.logBaro)
        if self.logBaro.valid:
            self.logBaro.data_received_cb.add_callback(self.print_baro_data)
            self.logBaro.start()
        else:
            print 'Could not setup log configuration for barometer after connection!'

        # Log Accelerometer
        self.logAccel = LogConfig("Accel",200)
        self.logAccel.add_variable("acc.x", "float")
        self.logAccel.add_variable("acc.y", "float")
        self.logAccel.add_variable("acc.z", "float")

        self.crazyflie.log.add_config(self.logAccel)

        if self.logAccel.valid:
            self.logAccel.data_received_cb.add_callback(self.print_accel_data)
            self.logAccel.start()
        else:
            print 'Could not setup log configuration for accelerometer after connection!'



        Thread(target=self.increasing_step).start()



    def print_baro_data(self, ident, data, logconfig):
        #logging.info("Id={0}, Barometer: asl={1:.4f}".format(ident, data["baro.aslLong"]))
        pass
 
    def print_stab_data(self, ident, data, logconfig):
        sys.stdout.write('Id={0}, Stabilizer: Roll={1:.2f}, Pitch={2:.2f}, Yaw={3:.2f}, Thrust={4:.2f}\r'.format(ident, data["stabilizer.roll"], data["stabilizer.pitch"], data["stabilizer.yaw"], data["stabilizer.thrust"]))

    def print_accel_data(self, ident, data, logconfig):
        #logging.info("Id={0}, Accelerometer: x={1:.2f}, y={2:.2f}, z={3:.2f}".format(ident, data["acc.x"], data["acc.y"], data["acc.z"]))
        pass


    def increasing_step(self):
        global key
        start_thrust = 10000 # (blades start to rotate after 10000)
        min_thrust = 10000
        max_thrust = 80000
        thrust_increment = 500

        start_roll = 0
        roll_increment = 30
        min_roll = -50
        max_roll = 50

        start_roll = 0
        roll_increment = 30
        min_roll = -50
        max_roll = 50

        start_pitch = 0
        pitch_increment = 30
        min_pitch = -50
        max_pitch = 50

        start_yaw = 0
        yaw_increment = 30
        min_yaw = -200
        max_yaw = 200

        stop_moving_count = 0

        pitch = start_pitch
        roll = start_roll
        thrust = start_thrust
        yaw = start_yaw
      
        # Start the keyread thread
        keyreader = KeyReaderThread()
        keyreader.start()

        sys.stdout.write('\r\nCrazyflie Status\r\n')
        sys.stdout.write('================\r\n')
        sys.stdout.write("Use 'w' and 's' for the thrust, 'a' and 'd' for yaw, 'i' and 'k' for pitch and 'j' and 'l' for roll. Stop flying with 'q'. Exit with 'e'.\r\n")


        while key != "e":
            if key == 'q':
                thrust = 0
                pitch = 0
                roll = 0
                yaw = 0
            elif key == 'w' and (thrust + thrust_increment <= max_thrust):
                thrust += thrust_increment
            elif key == 's' and (thrust - thrust_increment >= min_roll):
                thrust -= thrust_increment
            elif key == 'd' and (yaw + yaw_increment <= max_yaw):
                yaw += yaw_increment
                stop_moving_count = 0
            elif key == 'a' and (yaw - yaw_increment >= min_yaw):
                yaw -= yaw_increment
                stop_moving_count = 0
            elif key == 'l' and (roll + roll_increment <= max_roll):
                roll += roll_increment
                stop_moving_count = 0
            elif key == 'j' and (roll - roll_increment >= min_roll):
                roll -= roll_increment
                stop_moving_count = 0
            elif key == 'i' and (pitch + pitch_increment <= max_pitch):
                pitch += pitch_increment
                stop_moving_count = 0
            elif key == 'k' and (pitch - pitch_increment >= min_pitch):
                pitch -= pitch_increment
                stop_moving_count = 0
            elif key == "":
                # The controls are not being touch, get back to zero roll, pitch and yaw
                if stop_moving_count >= 40:
                    pitch = 0
                    roll = 0
                    yaw = 0
                else:
                    stop_moving_count += 1
            else:
                pass
            key = ""
            self.crazyflie.commander.send_setpoint(roll, pitch, yaw, thrust)


        self.crazyflie.commander.send_setpoint(0,0,0,0)
        # Make sure that the last packet leaves before the link is closed
        # since the message queue is not flushed before closing
        self.crazyflie.close_link()

TestFlight()
