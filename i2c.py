#!/usr/bin/python3
import serial, time

class ProtocolError(Exception):
    pass

class BusPirateI2C:
    def __init__(self, port="/dev/ttyUSB0", baud=115200):
        self._ser = serial.Serial(port=port, baudrate=baud, timeout=0.005)
        self._enterBitbang()
        self._enterI2C()
        self.setSpeed(3)
        
    def _enterBitbang(self):
        self._ser.read(100)
        for attempt in range(20):
            self._ser.write(b"\0")
            if self._ser.read(5) == b"BBIO1":
                return
            self._ser.read(100)
        raise ProtocolError("Could not enter bitbang")
    def _enterI2C(self):
        self._ser.write(b"\x02")
        resp = self._ser.read(4)
        if resp != b"I2C1":
            raise ProtocolError("Could not enter I2C mode. Got {}.".format(resp))
    def __del__(self):
        self._ser.write(b"\x00\x0f")

    def setSpeed(self, x):
        self._ser.write(bytes([0b01100000 | x]))
        x = self._ser.read(1)
        assert x == b"\x01"
    def start(self):
        self._ser.write(b"\x02")
        x = self._ser.read(1)
        assert x == b"\x01"
    def stop(self):
        self._ser.write(b"\x03")
        x = self._ser.read(1)
        assert x == b"\x01", x
    def readByteRaw(self):
        self._ser.write(b"\x04")
        r = self._ser.read(1)
        assert len(r) == 1, r
        return r
    def ack(self):
        self._ser.write(b"\x06")
        x = self._ser.read(1)
        assert x == b"\x01", x
    def readByte(self, ack=True):
        r = self.readByteRaw()
        if ack:
            self.ack()
        else:
            self.nack()
        return r
    def nack(self):
        self._ser.write(b"\x07")
        x = self._ser.read(1)
        assert x == b"\x01"
    def write(self, data):
        if type(data) == int:
            data = bytes([data])
        elif type(data) != bytes:
            data = bytes(data)
        #print("Writing {}".format(data))
        while len(data):
            chunk = data[:16]
            self._ser.write(bytes([(len(chunk)-1) | 0x10])+chunk)
            data = data[16:]
            x = self._ser.read(len(chunk)+1)
            assert len(x) == len(chunk)+1
            assert x[0] == 1, x
            for i,byte in enumerate(x[1:]):
                if byte != 0:
                    raise IOError("I2C write failed at byte {}".format(i))
            
    def power(self, pullup=True, power=True):
        cmd = 0b01000000
        if pullup:
            cmd |= (1<<2)
        if power:
            cmd |= (1<<3)
        self._ser.write(bytes([cmd]))
        x = self._ser.read(1)
        assert x == b"\x01", x

    def smbRead(self, dev, addr, n=1):
        self.start()
        self.write([dev & ~1, addr])
        self.stop()
        self.start()
        self.write([dev|1])
        r = []
        for i in range(n-1):
            x = self.readByte()
            r.append(x)
        x = self.readByte(False)
        r.append(x)
        self.stop()
        return b"".join(r)
    def smbRead16(self, dev, addr):
        data = self.smbRead(dev, addr, 2)
        return data[0] | (data[1]<<8)
    def smbWrite(self, dev, addr, data):
        self.start()
        self.write(bytes([dev & ~1, addr]) + bytes(data))
        self.stop()
    def smbWrite16(self, dev, addr, data):
        self.smbWrite(dev, addr, [data & 0xff, data >> 8])
        
if __name__ == "__main__":
    import sys
    sys.stdout.write("Powering on...")
    sys.stdout.flush()
    i2c = BusPirateI2C()
    i2c.power(pullup=False)
    i2c.power()
    time.sleep(0.2)
    print("OK")
    for i in range(2):
        print(i2c.smbRead(2, 1, 5))
