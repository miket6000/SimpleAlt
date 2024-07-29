#!/usr/bin/env python3
import struct
import serial

def swap32(i):
    return struct.unpack("<I", struct.pack(">I", i))[0]

offset = 0x10000
addresses = [offset]
rev_addr = 0

line = b''
addr = 0

with serial.Serial('/dev/ttyACM0', timeout=1) as ser:
    while (rev_addr != 0xffffffff):
        ser.write(f"R {addr} 4\n".encode())
        addr += 4
        s = ser.read(4 * 2)
        rev_addr = int(s, 16)
        if (rev_addr != 0xffffffff):
            addresses.append(swap32(rev_addr))

    for start in range(len(addresses)-1):
        for i in range(addresses[start],addresses[start+1],5):
            ser.write(f"R {i} 5\n".encode())
            label = chr(int(ser.read(2),16))
            data = swap32(int(ser.read(8),16))
            print(f"{start}, {i}, {label}, {data}")
            
