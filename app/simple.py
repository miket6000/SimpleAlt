#!/usr/bin/env python3
import serial
import csv
from datetime import datetime
import time

port = '/dev/ttyACM0'
now = datetime.now()
sample_rate = 0.02

offset = 0x10000
addresses = [offset]
addr = 0
a = 0
t = 0

""" 
The record length is fixed at 5 (char label + (u)int32_t). 
It's important that bytes_to_read does not exceed the maximum buffer size on the 
SimpleAlt, which is currently 64 bytes.
"""
num_records = 12
record_length = 5
bytes_to_read = num_records * record_length

data = []

with serial.Serial(port, timeout=1) as ser:
    
    # ensure we're not in interactive mode.
    ser.write(f"i\n".encode())
    i = 0
    # start reading the address block until we run out of recording addresses
    print("The following recordings were found:\n")
    while (a != 0xffffffff):
        ser.write(f"r {addr} 4\n".encode())
        addr += 4
        i += 1
        a = int.from_bytes(ser.read(4), "little")
        if (a != 0xffffffff):
            addresses.append(a)
            duration = (addresses[i] - addresses[i-1]) / record_length * sample_rate
            print(f"  [{i}] - duration {duration} seconds")

    input_str = input("\nWhich would you like to export?\n> ")
    try:
        start_address = addresses[int(input_str) - 1]
        end_address = addresses[int(input_str)] - 1
    except:
        print("Sorry the input wasn't recognised\n");
        exit()
    
    start_time = time.time()

    # Loop through the recordings and read the data out into a list of tuples
    for i in range(start_address, end_address, bytes_to_read):
        ser.write(f"r {i} {bytes_to_read}\n".encode())
        for r in range(num_records):
            label = chr(int.from_bytes(ser.read(1)))
            if (label == 'A'):
                altitude = int.from_bytes(ser.read(4), byteorder="little", signed=True) / 100
                data.append((t, altitude))
                t += sample_rate


print(f"Read {len(data)} records in {time.time() - start_time:.1f} seconds")
filename = f"SimpleAlt_flight_{input_str}_{now.strftime('%Y%m%d_%H%M%S')}.csv"

with open(filename, 'w', newline='') as out:
    csv_out = csv.writer(out)
    csv_out.writerow(['Time','Label','Value'])
    csv_out.writerows(data)

print(f"File saved as {filename}")
