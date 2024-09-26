import 'dart:typed_data';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'recording.dart';

class Altimeter {
  static const indexSize = 0x10000;
  static const altimeterFlashSize = 0x200000;
  static const altimeterBufferSize = 64;
  List<Recording> recordingList = [];
  SerialPort? port;
  int uid = 0;

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
      final disableInteractive = Uint8List.fromList("i\n".codeUnits);
      final getUID = Uint8List.fromList("UID\n".codeUnits);
      
      sp.write(disableInteractive, timeout: 1000);
      sp.read(1000, timeout: 1000); // clear the read buffer
      sp.write(getUID, timeout: 1000);
      uid = int.parse(String.fromCharCodes(sp.read(8, timeout: 1000)));
      sp.close();
    }
  }

  void altimeterGetBlock(List<int> buffer, int startAddress, int len) {
    if (len <= altimeterBufferSize) {
      final readCommand = Uint8List.fromList("r $startAddress $len\n".codeUnits);
      SerialPort sp = openPort();
      sp.write(readCommand, timeout: 1000);
      buffer.addAll(sp.read(len, timeout: 1000));
    }
  }

  void altimeterGetData(List<int> buffer, int startAddress, int len) {
    int address = startAddress;
    int endAddress = startAddress + len;
    int bytesToGet = 0;

    while(address < endAddress) {
      bytesToGet = endAddress - address;
      if (bytesToGet > altimeterBufferSize) {
        bytesToGet = altimeterBufferSize;
      }
      altimeterGetBlock(buffer, startAddress, len);
      address += bytesToGet;
    }
  }

  void parseAltimeterData(List<int> buffer) {
    int recordStartAddress = 0x10000;
    
    final altimeterIndex = buffer.sublist(0, Altimeter.indexSize-1);
    for (var settingIndex = 0; settingIndex < altimeterIndex.length; settingIndex += 5) {
      var label = String.fromCharCode(altimeterIndex[settingIndex]);
      if (label == String.fromCharCode(0xff)) {
        break;
      }
      var value = swapBytes(buffer.sublist(settingIndex+1, 4));

      settings[label]!.value = value;
    
      if (label == "r") {
        var length = recordStartAddress - value;
        recordingList.add(Recording(buffer.sublist(recordStartAddress, length)));
        recordStartAddress = value;
      }
    }
  }
}