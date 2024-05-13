#! /usr/bin/env python3
# load binary lib/pyeudaq.so
import pyeudaq
import pyvisa
from pyvisa import constants
from pyeudaq import EUDAQ_INFO, EUDAQ_ERROR
import time
import pyvisa as visa
import argparse


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
        self._scope = None
        self._rm = pyvisa.ResourceManager()
        self._scope = self._rm.open_resource(kwargs['resource'])
        self._scope.encoding = "latin-1"
        self._scope.baud_rate = kwargs['baud']
        self._scope.stop_bits = kwargs['stop_bit']
        self._scope.parity = kwargs['parity']
        self._scope.write('*RST')
        print("\n Resource identified as : ", self._scope.query('*IDN?'))

    def turnOn(self):
        self._scope.write(':OUTP ON')

    def turnOff(self):
        self._scope.write(':OUTP OFF')

    def setVoltage(self, voltage, maxCurr=5e-6):
        self._scope.write(f':SENS:CURR:PROT {maxCurr}')
        self._scope.write(f':SOUR:VOLT:LEV {voltage}')
        
    def readback(self):
        return self._scope.query_ascii_values(f':READ?')
        

    def availableRessources(self):
        return self._rm.list_resources()


class KeithleyPSProducer(pyeudaq.Producer):
    def __init__(self, name, runctrl):
        pyeudaq.Producer.__init__(self, name, runctrl)
        self.is_running = 0
        EUDAQ_INFO('New instance of KeithleyPSProducer')
        self._keithley = None
        self._maxCurrent = None
        self._voltage = .0
        self._logfile = ''
        self._logInterval = 10
        

    @exception_handler
    def DoInitialise(self):

        stop_bit_options = {'1': constants.StopBits.one, '1.5': constants.StopBits.one_and_a_half,
                            '2': constants.StopBits.two}
        parity_options = {'none': constants.Parity.none, 'even': constants.Parity.even, 'odd': constants.Parity.odd}

        ini = self.GetInitConfiguration()
        EUDAQ_INFO('DoInitialise')
        rsrc = ini.Get('resource', '')
        baud = ini.Get('baud', '9600')
        baud = int(baud)
        stops = ini.Get('stop_bit', '1')
        stops = stop_bit_options[stops]

        parity = ini.Get('parity', 'none')
        parity = parity_options[parity]
        self._logfile = ini.Get('file', '')
        self._logInterval = int(ini.Get('log_interval', '10'))
        
        if self._logfile:
            self._logfile = open(self._logfile, 'a')
        # self._logfile = open('test', 'w')

            
        

        self._keithley = KeithleyPS(resource=rsrc, baud=baud, stop_bit=stops, parity=parity)
        # print('available rsrcs', self._keithley.availableRessources())

    @exception_handler
    def DoConfigure(self):
        EUDAQ_INFO('DoConfigure')
        config = self.GetConfiguration()
        self._voltage = config.Get('voltage', '0')
        self._maxCurrent = config.Get('max_current', '5e-6')
        self._keithley.setVoltage(self._voltage, self._maxCurrent)
        self._keithley.turnOn()

    @exception_handler
    def DoStartRun(self):
        EUDAQ_INFO('DoStartRun')
        self.is_running = 1
        if self._logfile:
             self._logfile.write('\n\nNew run\n\n')

    @exception_handler
    def DoStopRun(self):
        EUDAQ_INFO('DoStopRun')
        self.is_running = 0
        #self._keithley.turnOff()

    @exception_handler
    def DoReset(self):
        EUDAQ_INFO('DoReset')
        self.is_running = 0

    @exception_handler
    def RunLoop(self):
        EUDAQ_INFO("Start of RunLoop in ExamplePyProducer")
        while self.is_running:
            #ev = pyeudaq.Event("RawEvent", "sub_name")
            if self._logfile:
                rb = self._keithley.readback()               
                self._logfile.write(f'{rb[0]}, {rb[1]}\n')
                self._logfile.flush()
        
            time.sleep(self._logInterval)
        EUDAQ_INFO("End of RunLoop in ExamplePyProducer")


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
