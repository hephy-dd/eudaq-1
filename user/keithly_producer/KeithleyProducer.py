#! /usr/bin/env python3
# load binary lib/pyeudaq.so
import pyeudaq
import pyvisa
from pyvisa import constants
from pyeudaq import EUDAQ_INFO, EUDAQ_ERROR
import time
from datetime import datetime
import pyvisa as visa
import argparse
import  numpy as np
import csv
import os
import threading


def exception_handler(method):
    def inner(*args, **kwargs):
        try:
            return method(*args, **kwargs)
        except Exception as e:
            EUDAQ_ERROR(str(e))
            raise e

    return inner


class KeithleyPS:
    def __init__(self, **kwargs):
        self._runnmb = None
        self._scope = None
        self._test = False
        self._recentMeasurement = None
        if self._test:
            return
        self._rm = pyvisa.ResourceManager()
        resources = self._rm.list_resources()
        print('available: ', resources)
        self._scope = self._rm.open_resource(kwargs['resource'])
        self._scope.encoding = "latin-1"
        self._scope.baud_rate = kwargs['baud']
        self._scope.stop_bits = kwargs['stop_bit']
        self._scope.parity = kwargs['parity']
        self._scope.write('*RST')
        print("\n Resource identified as : ", self._scope.query('*IDN?'))

    def turnOn(self):
        if not self._test:
            self._scope.write(':OUTP ON')

    def turnOff(self):
        if not self._test:
            self._scope.write(':OUTP OFF')

    def setVoltage(self, voltage, maxCurr=5e-6):
        if not self._test:
            self._scope.write(f':SENS:CURR:PROT {maxCurr}')
            self._scope.write(f':SOUR:VOLT:LEV {voltage}')

    def measure(self):
        if not self._test:
            self._recentMeasurement = self._scope.query('READ?').split(',')
            return self._recentMeasurement
        else:
            return ['5e3', '9e-12']

    def availableRessources(self):
        return self._rm.list_resources()


class KeithleyPSProducer(pyeudaq.Producer):
    def __init__(self, name, runctrl):
        pyeudaq.Producer.__init__(self, name, runctrl)
        self._is_running = False
        self._is_logging = False
        self._writer = None   
        self._log_thread = None
        self._runnmb = -1
        EUDAQ_INFO('New instance of KeithleyPSProducer')
        self._keithley = None
        self._maxCurrent = None
        self._logInterval = 1
        self._ivFile = None
        self._logPath = None

        self._currentVoltage = .0
        self._rampStep = .0
        self._rampSpeed = 1.0

    def ramp(self, targetVoltage):
        direction = +1
        if self._currentVoltage > targetVoltage: # determine direction we need to ramp (increase / decrease voltage)
            direction = -1

        # print('curr ', self._currentVoltage, 'trg ', targetVoltage)

        while not np.isclose(self._currentVoltage, targetVoltage):
            if abs(targetVoltage - self._currentVoltage) < self._rampStep:
                # step larger than diff between target and current U -> set to target directly (to not over- / undershoot)
                self._currentVoltage = targetVoltage
            else:
                self._currentVoltage += self._rampStep * direction # apply direction as sign to ramp step
            self._keithley.setVoltage(self._currentVoltage, self._maxCurrent)
            time.sleep(self._rampSpeed)

    @exception_handler
    def DoInitialise(self):

        stop_bit_options = {'1': constants.StopBits.one, '1.5': constants.StopBits.one_and_a_half,
                            '2': constants.StopBits.two}
        parity_options = {'none': constants.Parity.none, 'even': constants.Parity.even, 'odd': constants.Parity.odd}

        ini = self.GetInitConfiguration()
        EUDAQ_INFO('DoInitialise')
        rsrc = ini.Get('resource', '')
        baud = int(ini.Get('baud', '9600'))
        stops = ini.Get('stop_bit', '1')

        stops = stop_bit_options[stops]
        parity = ini.Get('parity', 'none')
        parity = parity_options[parity]

        self._logPath = ini.Get('log_path', '')
                            
        self._logInterval = float(ini.Get('log_interval', '1000')) * 1e-3

        self._keithley = KeithleyPS(resource=rsrc, baud=baud, stop_bit=stops, parity=parity)
        # print('available rsrcs', self._keithley.availableRessources())

    @exception_handler
    def DoConfigure(self):
        config = self.GetConfiguration()
        targetVoltage = float(config.Get('voltage', '0'))
        self._rampStep = abs(float(config.Get('ramp_step', '5')))
        self._rampSpeed = abs(float(config.Get('ramp_speed', '1000')) * 1e-3) #given in ms, we use sec though

        self._maxCurrent = float(config.Get('max_current', '5e-6'))
        self._is_logging = True
        self._keithley.turnOn()
        self.ramp(targetVoltage)
        self.openLogFile()
        # only create/start if not already running
        if self._log_thread is None or not self._log_thread.is_alive():
            self._log_thread = threading.Thread(
                target=self.logWorker,
                daemon=True
            )
            self._log_thread.start()

        # self._keithley.setVoltage(self._targetVoltage, self._maxCurrent)


    @exception_handler
    def DoStartRun(self):
        self._is_running = True
        self._runnmb = self.GetRunNumber()        
        self.openLogFile()
        # if self._ivFile:
        #     self._ivFile.write(f'\n\nNew Run at {datetime.datetime.now().strftime("%d.%m.%Y %H:%M:%S")}\n\n')

    @exception_handler
    def DoStopRun(self):
        self._is_running = False
        if self._ivFile:
            self._ivFile.write('\n\n#STOPPED\n\n\n')
            self._ivFile.flush()

    @exception_handler
    def DoReset(self):
        self._is_running = False
        self._is_logging = False
        self.ramp(0.0)
        self._keithley.turnOff()
        if self._log_thread:
            self._log_thread.join()
        if self._ivFile:
            self._ivFile.close()

    @exception_handler
    def DoStatus(self):
        if not self._is_logging:
            return
        if not self._keithley:
            return
        iv = self._keithley._recentMeasurement
        if iv is None:
            return
        self.SetStatusTag('U [V]', iv[0])
        self.SetStatusTag('I [A]', iv[1])

    @exception_handler
    def RunLoop(self):
        while self._is_running:
            time.sleep(self._logInterval)
            # response = self._keithley.measure()
            # data = response.split(',')

            # voltage = float(data[0])
            # current = float(data[1])
            # print('IV: ', response)
            # if self._writer:
            #     self._writer.writerow([voltage, current, self.GetRunNumber()])

    def logWorker(self):
        while self._is_logging:
            time.sleep(self._logInterval)
            data = self._keithley.measure()

            voltage = float(data[0])
            current = float(data[1])
            print(f'U = {voltage:.2e}V, I = {current:.2e}A')
            if self._writer:
                self._writer.writerow([self._runnmb, datetime.now().strftime("%Y.%m.%d %H:%M:%S"), voltage, current])

    def openLogFile(self):
        if not self._logPath:
            return
        self._ivFile = open(f'{self._logPath}/run{str(self._runnmb).zfill(6)}.csv', 'a')
        print('opened ', self._ivFile)
        self._writer = csv.writer(self._ivFile)
        headers = ['Run Number', 'Time', 'Voltage [V]', 'Current [A]']                
        self._writer.writerow(headers)



def parse_arguments():
    parser = argparse.ArgumentParser(description="EUDAQ producer to control Keithley Power Supply via PyVisa")
    parser.add_argument('-r', '--run-control-address', default='localhost', help="Specify the run control address ("
                                                                                 "default: localhost)")
    parser.add_argument('-n', '--runtime-name', default='keithleyPS', help="Specify the runtime name (default: "
                                                                           "keithleyPS)")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_arguments()
    prod = KeithleyPSProducer(args.runtime_name, args.run_control_address)
    print("connecting to runcontrol in " + args.run_control_address)
    prod.Connect()
    time.sleep(2)
    while prod.IsConnected():
        time.sleep(1)
