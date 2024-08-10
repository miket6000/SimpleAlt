import 'dart:developer';
import 'dart:typed_data';
import 'package:flutter_libserialport/flutter_libserialport.dart';

class Recording {
  int index = 0;
  int startAddress = 0;
  int endAddress = 0;
  List <double> data = [];

  Recording({required this.index, required this.startAddress, required this.endAddress});

  double getDuration() {
    return (endAddress - startAddress) / 5 * 0.02;
  }

  bool hasData() {
    return (data.isNotEmpty);
  }
}

class Altimeter {
  static const firstAdress = 0x10000;
  SerialPort? port;
  List<Recording> recordings = []; 

  bool findAltimeter() {
    for (final address in SerialPort.availablePorts) {
      if (SerialPort(address).productName == "STM32 Virtual ComPort") {
        port = SerialPort(address);
        return true;
      }
    }
    return false;
  }

  SerialPort openPort() {
    port!.openReadWrite();
    SerialPortConfig config = port!.config;
    config.setFlowControl(SerialPortFlowControl.none);
    port!.config = config;
    return port!;
  }

  Altimeter() {
    if (findAltimeter()) {
      SerialPort sp = openPort();
    
      final command = Uint8List.fromList("i\n".codeUnits);
      sp.write(command, timeout: 1000);
      sp.close();
    }
  }

  Recording fetchRecording(int index) {
    Uint8List command = Uint8List(16);
    var response = Uint8List(61);
    
    if (findAltimeter()) {
      if (index < recordings.length) {
        if (!recordings[index].hasData()) {
          //download the data
          SerialPort sp = openPort(); 
          int address = recordings[index].startAddress;
          while(address < recordings[index].endAddress) {
            command = Uint8List.fromList("r ${address} 60\n".codeUnits);
            sp.write(command, timeout: 1000);
            response = sp.read(60, timeout: 4000);
            for(int i = 0; i < 60; i+=5) {
              if (address + i < recordings[index].endAddress) {
                recordings[index].data.add(response.buffer.asByteData().getInt32(i + 1, Endian.little) / 100.0);          
              }
            }
            address += 60;
          } 
          sp.close();
        }
      }
      return recordings[index];
    } else {
      return Recording(index: 0, startAddress: 0, endAddress: 0);
    }
  }


  
  // retrieves a list of all the recordings on the altimeter and creates a Recording instance for each one.
  // Note, this function does not populate the data in the Recording instance as this takes a long time, so
  // is expected to be done on demand by calling fetchRecording()
  List<Recording> getRecordingList() {
    var command = Uint8List.fromList("r 0 4\n".codeUnits);
    var response = Uint8List(4);
    var startAddress = 0x10000;
    var endAddress = 0x10000;
    var count = 0;
    
    SerialPort sp = openPort();
    recordings = [];
    while (endAddress != 0xFFFFFFFF && count < 255) {
      command = Uint8List.fromList("r ${count*4} 4\n".codeUnits);
      sp.write(command, timeout: 1000);
      response = sp.read(4, timeout: 1000);
      if (response.length == 4) {
        endAddress = response.buffer.asByteData().getUint32(0, Endian.little);
        if (endAddress != 0xFFFFFFFF) {
          recordings.add(Recording(index: count, startAddress: startAddress, endAddress: endAddress));
          startAddress = endAddress;
        }     
      } else {
        log("There was an error attempting to read address ${count * 4}");
      }

      count++;
    }
    sp.close();
    
    return recordings;
  }
}