#!/usr/bin/env python3
import serial
import csv
from datetime import datetime
import time

start_time = time.time()
now = datetime.now() # current date and time
date_time = now.strftime("%m/%d/%Y, %H:%M:%S")
filename = f"SimpleAlt_log_{now.strftime('%Y%m%d_%H%M%S')}.csv"

offset = 0x10000
addresses = [offset]
addr = 0
a = 0

""" 
The record length is fixed at 5 (char label + (u)int32_t). 
It's important that bytes_to_read does not exceed the maximum buffer size 
"""
num_records = 12
record_length = 5
bytes_to_read = num_records * record_length

data = []

with serial.Serial('/dev/ttyACM0', timeout=1) as ser:
    
    # ensure we're not in interactive mode.
    ser.write(f"i\n".encode())

    # start reading the address block until we run out of recording addresses
    while (a != 0xffffffff):
        ser.write(f"r {addr} 4\n".encode())
        addr += 4
        a = int.from_bytes(ser.read(4), "little")
        if (a != 0xffffffff):
            addresses.append(a)

    print(f"Recordings discovered: {len(addresses)-1}")

    # Loop through the recordings and read the data out into a list of tuples
    for recording in range(len(addresses)-1):
        for i in range(addresses[recording], addresses[recording+1], bytes_to_read):
            ser.write(f"r {i} {bytes_to_read}\n".encode())
            for r in range(num_records):
                label = chr(int.from_bytes(ser.read(1)))
                if (label != chr(0xff)):
                    data.append((recording, label, int.from_bytes(ser.read(4), byteorder="little", signed=True)))

print(f"Total records read: {len(data)} in {time.time() - start_time} seconds")

with open(filename, 'w', newline='') as out:
    csv_out = csv.writer(out)
    csv_out.writerow(['Recording','Label','Value'])
    csv_out.writerows(data)

print(f"File saved as {filename}")
