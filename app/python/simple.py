import serial
#!/usr/bin/env python3
import csv
from datetime import datetime
import time

port = '/dev/ttyACM0'
now = datetime.now()
sample_rate = 0.02
ground_level = 0

offset = 0x10000
addresses = [offset]
addr = 0
a = 0
t = 0

record_types = {
    'A': { 'size': 4 , 'name': 'Altitude' ,'order':"little", 'signed':True , 'factor':100 },
    'P': { 'size': 4 , 'name': 'Presure'  ,'order':"little", 'signed':True, 'factor':100  },
    'T': { 'size': 2 , 'name': 'Temperature'  ,'order':"little", 'signed':True, 'factor':100  },
    'V': { 'size': 2 , 'name': 'Voltage'  ,'order':"little", 'signed':True, 'factor':1000 },
}

"""
The record length is fixed at 5 (char label + (u)int32_t).
It's important that bytes_to_read does not exceed the maximum buffer size on the
SimpleAlt, which is currently 64 bytes.
"""
num_records = 4
items_per_record = 4
record_length = 16 # 5 + 5 + 3 + 3
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
            print(f"  [{i}] \tduration {duration:.2f} seconds")

    print(f"\nThese flights use up {addresses[-1] / 2**21 * 100:.2f}% of the available flash\n")
    input_str = input("Which would you like to export?\n> ")
    try:
        start_address = addresses[int(input_str) - 1]
        end_address = addresses[int(input_str)] - 1
    except:
        print("Sorry the input wasn't recognised\n");
        exit()

    start_time = time.time()

    titles = ['Time']

    # Loop through the recordings and read the data out into a list of tuples
    for address in range(start_address, end_address, bytes_to_read):
        ser.write(f"r {address} {bytes_to_read}\n".encode())
        for r in range(num_records):
            row = [t]
            for item in range(items_per_record):
                label = chr(int.from_bytes(ser.read(1)))
                recordType = record_types.get(label,None)

                # all records must be known, abort if not
                if recordType is None:
                    print(f"Early exit at @{address}, instead of {end_address}");
                    break;

                value = int.from_bytes(ser.read(recordType['size']),
                        byteorder=recordType['order'],
                        signed=recordType['signed'] ) / recordType['factor']
                if len(titles) <= items_per_record:
                    titles.append(recordType['name'])

                if (label == 'A'):
                    # assume the first entry is "ground"
                    if (address == start_address and r == 0):
                        ground_level = 0 #value

                    row.append(value - ground_level)
                else:
                    row.append(value)

            data.append(row)
            t += sample_rate
            print(row, titles)



print(f"Read {len(data)} records in {time.time() - start_time:.1f} seconds")
filename = f"SimpleAlt_flight_{input_str}_{now.strftime('%Y%m%d_%H%M%S')}.csv"

csv_data = [titles]

for record in data:
    row = []
    for item in record:
        row.append(f"{item:0.3f}")

    csv_data.append(row)

with open(filename, 'w', newline='') as out:
    csv_out = csv.writer(out)
    csv_out.writerows(csv_data)

print(f"File saved as {filename}")
