from twisted.internet import reactor
from twisted.application import service
from twisted.internet.serialport import SerialPort
from twisted.python import log
from twisted.python.logfile import LogFile

from protocol import RFIDProtocol


class SerialService(service.Service):
    def __init__(self, reactor=None, name=None, baud=9600, log=None):
        self.reactor = reactor
        self.name = name
        self.baud = baud
        self.log = log

    def startService(self,reactor=reactor, name=None, log=None):
        self.controller = SerialPort(RFIDProtocol(), 
                self.name, self.reactor, baudrate=self.baud)



# Log file maxes at 1mb, and limit it to 1 of them, store them in /tmp
#logfile = LogFile('rfid.log','/tmp',1000000,None,0)
#log.startLogging(logfile)


application = service.Application('RFID Controller')
multiService = service.MultiService()
controllerService = SerialService(reactor, '/dev/ttyACM0', 9600, None)
controllerService.setServiceParent(multiService)
multiService.setServiceParent(application)


