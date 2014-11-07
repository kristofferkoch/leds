#!/usr/bin/python3

import i2c

class Programmer:
    def __init__(self, i2c):
        self._i2c = i2c
        if self.getVersion() != 1:
            raise NotImplementedError("Only supports version 1")
        if self.getSignature()[0:3] != b"\x1e\x95\x0f":
            raise NotImplementedError("Only supports atmega328p")
    def getStatus(self):
        return self._i2c.smbRead(0x2, 0x0)[0]
    def getVersion(self):
        return self._i2c.smbRead16(0x2, 0x5)
    def getSignature(self):
        return self._i2c.smbRead(0x2, 0x1, 4)
    
    def getAddr(self):
        return self._i2c.smbRead16(0x2, 0x7)
    def setAddr(self, addr):
        self._i2c.smbWrite16(0x2, 0x7, addr)
        
    def cmd(self, cmd):
        self._i2c.smbWrite(0x2, 0x9, [cmd])
    def setData(self, data):
        self._i2c.smbWrite(0x2, 0xa, data)
    def getData(self, n):
        return self._i2c.smbRead(0x02, 0xa, n)
    def setLength(self, length):
        self._i2c.smbWrite16(0x2, 0xa, length)
    def getLength(self):
        return self._i2c.smbRead16(0x02, 0x0a)
    def getResult(self):
        return self._i2c.smbRead16(0x02, 0x0c)
    def cmdReset(self):
        self.cmd(0x01)
    def cmdErase(self, page):
        self.setAddr(page)
        self.cmd(0x02)
    def cmdWrite(self, page):
        self.setAddr(page)
        self.cmd(0x03)
    def cmdCrc(self, addr, length):
        self.setAddr(addr)
        self.setLength(length)
        self.cmd(0x04)
        return self.getResult()
    
if __name__ == "__main__":
    import sys, time
    sys.stdout.write("Powering on...")
    sys.stdout.flush()
    i2c = i2c.BusPirateI2C()
    i2c.power()
    time.sleep(0.1)
    print("OK")
    prg = Programmer(i2c)
    
    prg.setAddr(0x7c00)
    print(hex(prg.getAddr()))
    print(prg.getData(512))
    print(hex(prg.getAddr()))

