from twisted.internet import reactor
from twisted.internet.protocol import Factory
from twisted.protocols.basic import LineReceiver
from twisted.web.client import getPage

class RFIDProtocol(LineReceiver):
    def lineReceived(self, line):
        door, binary = line.split(':')
        facility_code, rfid = int(binary[1:9], 2), int(binary[10:25], 2)
        if self.open_sesame(door, rfid):
            self.transport.write("%s:1\n" % door) # opens the door
            reactor.callLater(5, lambda: self.transport.write, "%s:0" % door) # close door 5 seconds later
            d = getPage('http://127.0.0.1:5000/doors/accesslog/%s:%s/' % (door, rfid))

    def open_sesame(self, door, rfid):
        try:
            with open('/tmp/door_%s.txt' % door, 'r') as f:
                lines = f.readlines()
                for line in lines:
                    if rfid in line:
                        return True
                return False
        except IOError:
            # File doesn't exist or something
            return False

