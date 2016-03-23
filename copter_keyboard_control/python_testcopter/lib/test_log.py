#!/usr/bin/python
import logging
import cflib.crtp
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import Log, LogVariable, LogConfig
 
logging.basicConfig(level=logging.DEBUG)

class Main:
    """
    Class is required so that methods can access the object fields.
    """
    def __init__(self):
        """
        Connect to Crazyflie, initialize drivers and set up callback.
        The callback takes care of logging the accelerometer values.
        """
        self.crazyflie = cflib.crazyflie.Crazyflie()
        print 'Init drivers'
        cflib.crtp.init_drivers()
 
        print 'Search available interfaces'
        available = cflib.crtp.scan_interfaces()

        radio = False
        for i in available:
            print "Interface with URI [%s] found and name/comment [%s]" % (i[0], i[1])
            if 'radio' in i[0]:
                radio = True
                dev = i[0]
                self.crazyflie.open_link(dev)
                break

        if not radio:
            print 'No quadcopter detected'
            exit(-1)

        # Set up the callback when connected
        self.crazyflie.connectSetupFinished.add_callback(self.connectSetupFinished)


    def connectSetupFinished(self, linkURI):
        """
        Configure the logger to log accelerometer values and start recording.
 
        The logging variables are added one after another to the logging
        configuration. Then the configuration is used to create a log packet
        which is cached on the Crazyflie. If the log packet is None, the
        program exits. Otherwise the logging packet receives a callback when
        it receives data, which prints the data from the logging packet's
        data dictionary as logging info.
        """
        ## Set stabilizer logging config
        ##stab_log_conf = LogConfig("Stabalizer", 200)
        ##stab_log_conf.add_variable("stabilizer.roll", "float")
        ##stab_log_conf.add_variable("stabilizer.pitch", "float")
        ##stab_log_conf.add_variable("stabilizer.yaw", "float")
        ##stab_log_conf.add_variable("stabilizer.thrust", "uint16_t")
 
        ### Now that the connection is established, start logging
        ##self.crazyflie.log.add_config(stab_log_conf)
 
        ##if stab_log_conf.valid:
        ##    stab_log_conf.data_received_cb.add_callback(self.log_stab_data)
        ##    stab_log_conf.start()
        ##else:
        ##    print("acc.x/y/z not found in log TOC")


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
            logger.warning("Could not setup log configuration for stabilizer after connection!")

        # Set barometer
        self.logBaro = LogConfig("Baro", 200)
        self.logBaro.add_variable("baro.aslLong", "float")

        self.crazyflie.log.add_config(self.logBaro)
        if self.logBaro.valid:
            self.logBaro.data_received_cb.add_callback(self.print_baro_data)
            self.logBaro.start()
        else:
            logger.warning("Could not setup log configuration for barometer after connection!")

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
            logger.warning("Could not setup log configuration for accelerometer after connection!")

    def print_baro_data(self, ident, data, logconfig):
        logging.info("Id={0}, Barometer: asl={1:.4f}".format(ident, data["baro.aslLong"]))
 
    def print_stab_data(self, ident, data, logconfig):
        logging.info("Id={0}, Stabilizer: Roll={1:.2f}, Pitch={2:.2f}, Yaw={3:.2f}, Thrust={4:.2f}".format(ident, data["stabilizer.roll"], data["stabilizer.pitch"], data["stabilizer.yaw"], data["stabilizer.thrust"]))

    def print_accel_data(self, ident, data, logconfig):
        logging.info("Id={0}, Accelerometer: x={1:.2f}, y={2:.2f}, z={3:.2f}".format(ident, data["acc.x"], data["acc.y"], data["acc.z"]))



Main()
