import 'dart:io';
import 'dart:developer' as dev;
import 'package:flutter/foundation.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'recording.dart';

class Altimeter {
  static Altimeter? _instance;
  static const indexSize = 0x10000;
  static const flashSize = 0x200000;
  static const altimeterInternalBufferSize = 64;
  static const uidLength = 16;
  double flashUtilization = 0;
  List<Recording> recordingList = [];
  Uint8List buffer = Uint8List(flashSize);
  
  SerialPort? port;
  int uid = 0;

  factory Altimeter() {
      _instance ??= Altimeter._constructor();
    return _instance!;
  }

  bool findAltimeter() {
    for (final address in SerialPort.availablePorts) {
      if (SerialPort(address).productName == "STM32 Virtual ComPort") {
        port = SerialPort(address);
        return true;
      }
    }
    return false;
  }

  SerialPort? openPort() {
    try { 
      port!.openReadWrite();
      SerialPortConfig config = port!.config;
      config.setFlowControl(SerialPortFlowControl.none);
      port!.config = config;
    } on Error {
      return null;
    }
    return port;
  }

  int connect() {
    uid = 0;
    if (findAltimeter()) {
      SerialPort? sp = openPort();
      if (sp != null) {
        final disableInteractive = Uint8List.fromList("i\n".codeUnits);
        final getUID = Uint8List.fromList("UID\n".codeUnits);
        buffer.fillRange(0, flashSize, 0xff);
      
        sp.write(disableInteractive, timeout: 1000);
        sp.read(1000, timeout: 1000); // clear the read buffer
        sp.write(getUID, timeout: 1000);
        var rawRead = sp.read(uidLength, timeout: 1000);
        uid = int.parse(String.fromCharCodes(rawRead));
        sp.close();
      }
    }
    return uid;        
  }

  Altimeter._constructor() {
    _instance = this;
  }

  int sendCommand(String cmd) {
    SerialPort? sp = openPort();
    String response = "";
    if (sp != null) {
      final cmdList = Uint8List.fromList([...cmd.codeUnits,'\n'.codeUnitAt(0)]);
      sp.write(cmdList, timeout: 1000);
      response = sp.read(uidLength, timeout: 1000).toString();
      sp.close();
    }
    
    if (response.compareTo("OK\n") == 1) {
      return 0;
    }
    return -1;
  }

  /* 
   * Copies 'len' bytes from the altimeter into the internal buffer starting at 'startAdderss'.
   * 'len' must be less than the length of the altimeters intern buffer and must not extend beyond 
   * the end of flash or the request is ignored.
   */
  int getBlock(int startAddress, int len) {
    if (startAddress > flashSize) { return 0; }
    if (len < 0 || len > altimeterInternalBufferSize) { return 0; }
    
    final bytesToRead = startAddress + len < flashSize ? len : flashSize - startAddress;
    final readCommand = Uint8List.fromList("r $startAddress $bytesToRead\n".codeUnits);
    
    port!.write(readCommand, timeout: 1000);
    Uint8List received = port!.read(len, timeout: 1000); 
    
    if (received.length != len) {
      dev.log("Insuffucient bytes received. Read ${received.length}, expected $len");
    }
    
    buffer.setRange(startAddress, startAddress + received.length, received);
    
    return received.length;
  }

  /*
   * Copies 'len' bytes from the altimeter to the internal buffer using multiple calls to getBlock(). 
   * Attempts to copy beyond the end of flash will result in the copy stopping when the end of flash is reached.
   */
  int getData(int startAddress, int len) {
    int bytesRead = 0;
    if (startAddress > flashSize) { return 0; }
    if (len < 0) { return 0; }

    final int endAddress = (startAddress + len < flashSize ? startAddress + len : flashSize);
    int address = startAddress;
    int bytesToGet = 0;
    openPort();

    while(address < endAddress) {
      bytesToGet = endAddress - address;
      if (bytesToGet > altimeterInternalBufferSize) {
        bytesToGet = altimeterInternalBufferSize;
      }
      bytesRead += getBlock(address, bytesToGet);
      address += bytesToGet;
    }

    port!.close();
    return bytesRead;
  }

  /* 
   * saves the binary flash data of the altimeter to a file for future use.
   */
  void save() {  
    File("${uid.toRadixString(16)}.dump").writeAsBytesSync(buffer, flush:true);
  }

  /* 
   * If no filename is provided sync will check for a file based on the UID and download
   * the data from the altimeter if the index differs from the file, or will just use the file.
   * If a filename is provided, sync will just load the file without checking the altimeter.@override
   */
  void sync({String? filename}) {
    Uint8List fileBuffer;
    if (filename == null) {
      filename = "${uid.toRadixString(16)}.dump"; 
      var file = File(filename);
      getData(0, indexSize);
      int endOfData = findDataEnd();
      if (file.existsSync()) {
        fileBuffer = File(filename).readAsBytesSync();
        if(listEquals(buffer.sublist(0, indexSize), fileBuffer.sublist(0, indexSize))) {
          buffer.setAll(0, fileBuffer);
        } else {
          getData(indexSize, endOfData - indexSize);
        }
      } else {
        getData(indexSize, endOfData - indexSize);
      }

      save();
    } else {
      var fileBuffer = File(filename).readAsBytesSync();
      buffer.setAll(0, fileBuffer);
    }
  }

  int findDataEnd() {
    const int recordLength = 5;
    const int labelLength = 1;
    int eof = indexSize; 

    for (int settingIndex = 0; settingIndex + recordLength < indexSize - 1; settingIndex += recordLength) {
      String label = String.fromCharCode(buffer[settingIndex]);
      if (label == String.fromCharCode(0xff)) {
        break;
      }
      var reversedList = buffer.sublist(settingIndex + labelLength, settingIndex + recordLength);
      int value = swapBytes(reversedList);
    
      if (label == "r") {
        eof = value;
      }
    }
    return eof;
  }


  /* 
   * Steps through the data reconstructing recordings based on the relevant settings as they
   * were at the time the record was made. The recordings are available in the 'recordingsList' 
   * variable of the Altimeter class.
   */ 
  void parseData() {
    const int recordLength = 5;
    const int labelLength = 1;
    int recordStartAddress = indexSize; 

    // clean out old data
    recordingList.clear();

    for (int settingIndex = 0; settingIndex + recordLength < indexSize - 1; settingIndex += recordLength) {
      String label = String.fromCharCode(buffer[settingIndex]);
      if (label == String.fromCharCode(0xff)) {
        break;
      }
      var reversedList = buffer.sublist(settingIndex + labelLength, settingIndex + recordLength);
      int value = swapBytes(reversedList);

      settings[label]!.value = value;
    
      if (label == "r") {
        recordingList.add(Recording(buffer.sublist(recordStartAddress, value)));
        recordStartAddress = value;
      }
    }
    flashUtilization = (recordStartAddress - indexSize) / (flashSize - indexSize);
  }

  bool saveSetting(String setting) {
    SerialPort? sp = openPort(); 
    if (sp == null) {
      return false;
    }

    final setSetting = Uint8List.fromList("SET $setting ${settings[setting]!.value}\n".codeUnits);
    sp.write(setSetting, timeout: 1000);
    sp.close();
    return true;
  }

}