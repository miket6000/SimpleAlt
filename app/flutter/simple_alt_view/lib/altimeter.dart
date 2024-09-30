import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'recording.dart';

class Altimeter {
  static Altimeter? _instance;
  static const indexSize = 0x10000;
  static const flashSize = 0x200000;
  static const altimeterInternalBufferSize = 64;
  static const uidLength = 16;
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

  SerialPort openPort() {
    port!.openReadWrite();
    SerialPortConfig config = port!.config;
    config.setFlowControl(SerialPortFlowControl.none);
    port!.config = config;
    return port!;
  }

  Altimeter._constructor() {
    if (findAltimeter()) {
      SerialPort sp = openPort(); 
      final disableInteractive = Uint8List.fromList("i\n".codeUnits);
      final getUID = Uint8List.fromList("UID\n".codeUnits);
      buffer.fillRange(0, flashSize, 0x55);
      
      sp.write(disableInteractive, timeout: 1000);
      sp.read(1000, timeout: 1000); // clear the read buffer
      sp.write(getUID, timeout: 1000);
      var rawRead = sp.read(uidLength, timeout: 1000);
      uid = int.parse(String.fromCharCodes(rawRead));
      sp.close();
    }

    _instance = this;
  }

  /* 
   * Copies 'len' bytes from the altimeter into the internal buffer starting at 'startAdderss'.
   * 'len' must be less than the length of the altimeters intern buffer and must not extend beyond 
   * the end of flash or the request is ignored.
   */
  void getBlock(int startAddress, int len) {
    if (startAddress > flashSize) { return; }
    if (len < 0) { return; }
    
    if (len <= altimeterInternalBufferSize && startAddress + len < (flashSize - 1)) {
      final readCommand = Uint8List.fromList("r $startAddress $len\n".codeUnits);
      port!.write(readCommand, timeout: 1000);
      buffer.setRange(startAddress, startAddress + len, port!.read(len, timeout: 1000));
    }
  }

  /*
   * Copies 'len' bytes from the altimeter to the internal buffer using multiple calls to getBlock(). 
   * Attempts to copy beyond the end of flash will result in the copy stopping when the end of flash is reached.
   */
  void getData(int startAddress, int len) {
    if (startAddress > flashSize) { return; }
    if (len < 0) { return; }

    final int endAddress = (startAddress + len < flashSize ? startAddress + len : flashSize);
    int address = startAddress;
    int bytesToGet = 0;
    openPort();

    while(address < endAddress) {
      bytesToGet = endAddress - address;
      if (bytesToGet > altimeterInternalBufferSize) {
        bytesToGet = altimeterInternalBufferSize;
      }
      getBlock(address, bytesToGet);
      address += bytesToGet;
    }
    port!.close();
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
      if (file.existsSync()) {
        fileBuffer = File(filename).readAsBytesSync();
        getData(0, indexSize);
        if(listEquals(buffer.sublist(0, indexSize), fileBuffer.sublist(0, indexSize))) {
          buffer.setAll(0, fileBuffer);
        } else {
          getData(indexSize, flashSize - indexSize);
        }
      } else {
        getData(0, flashSize);
      }
      save();
    } else {
      var fileBuffer = File(filename).readAsBytesSync();
      buffer.setAll(0, fileBuffer);
    }
  }

  /* 
   * Steps through the data reconstructing recordings based on the relevant settings as they
   * were at the time the record was made. The recordings are available in the 'recordingsList' 
   * variable of the Altimeter class.
   */ 
  void parseData() {
    const int recordLength = 5;
    int recordStartAddress = indexSize; 

    for (int settingIndex = 0; settingIndex + recordLength < indexSize - 1; settingIndex += recordLength) {
      String label = String.fromCharCode(buffer[settingIndex]);
      if (label == String.fromCharCode(0xff)) {
        break;
      }
      var reversedList = buffer.sublist(settingIndex + 1, settingIndex + 5);
      int value = swapBytes(reversedList);

      settings[label]!.value = value;
    
      if (label == "r") {
        recordingList.add(Recording(buffer.sublist(recordStartAddress, value)));
        recordStartAddress = value;
      }
    }
  }
}