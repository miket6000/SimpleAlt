import 'dart:typed_data';
import 'package:flutter/material.dart';

const double tickDuration = 0.01;

class Setting {
  final String title;
  bool configurable;
  int value;
  Setting({required this.title, this.value = 0, this.configurable = true});
}

class Unit {
  final String title;
  final double slope;
  final double offset;
  const Unit({required this.title, required this.slope, required this.offset});
}

class Record {
  final String title;
  final int length;
  final String setting;
  final String unit;
  final int precision;  //CSV print prescision
  double time = 0;
  bool plot;            //Do or donot show in graphs
  Color colour;         //Colour of graph line and axis text
  Record({required this.title, required this.setting, required this.length, required this.unit, required this.precision, this.plot=true, this.colour=Colors.blue});
}

final Map<String, Setting> settings = {
  "a": Setting(title:"Altitude Sample Rate"),
  "p": Setting(title:"Pressure Sample Rate"),
  "t": Setting(title:"Temperature Sample Rate"),
  "v": Setting(title:"Voltage Sample Rate"),
  "s": Setting(title:"Status Sample Rate"), 
  "m": Setting(title:"Maximum Altitude", configurable:false),
  "o": Setting(title:"Power Off Timeout"),
  "r": Setting(title:"Recording Start Address", configurable:false),
};

const Map<String, Unit> units = {
  "V":    Unit(title:"Voltage",       slope:1000,         offset:0),
  "m":    Unit(title:"Altitude",      slope:100,          offset:0),
  "ft":   Unit(title:"Altitude",      slope:30.48,        offset:0),
  "hPa":  Unit(title:"Pressure",      slope:100,          offset:0),
  "psi":  Unit(title:"Pressure",      slope:6894.75729,   offset:0),
  "°C":   Unit(title:"Temperature",   slope:100,          offset:0),
  "°F":   Unit(title:"Temperature",   slope:180,          offset:32),
  "-":    Unit(title:"Status",        slope:1,            offset:0),
};

final Map<String, Record> records = {
  "A": Record(title:"Altitude",    setting:"a", length:4, unit:"m",   precision:1, colour:Colors.blue,    plot:true),
  "P": Record(title:"Pressure",    setting:"p", length:4, unit:"hPa", precision:2, colour:Colors.orange,  plot:false),
  "T": Record(title:"Temperature", setting:"t", length:2, unit:"°C",  precision:1, colour:Colors.red,     plot:true),
  "V": Record(title:"Voltage",     setting:"v", length:2, unit:"V",   precision:2, colour:Colors.purple,  plot:false),
  "S": Record(title:"Status",      setting:"s", length:1, unit:"-",   precision:0, colour:Colors.green,   plot:false),
};

int swapBytes(Uint8List bytes) {
  var value = 0;
  if (bytes.length == 1) {
    value = bytes[0].toInt();
  } else if (bytes.length == 2) {
    value = (bytes[1].toInt() << 8) + bytes[0].toInt();
  } else  if (bytes.length == 4) {
    value = (bytes[3].toInt() << 24) + 
            (bytes[2].toInt() << 16) + 
            (bytes[1].toInt() << 8) + 
            bytes[0].toInt();
  } else {
    throw "Invalid number of bytes"; 
  }

  return value;
}

class Recording {
  var index = 0;
  var maxAltitude = 0.0;
  var groundLevel = double.infinity;

  Map<String, List<List<double>>> values = {};

  double getDuration(){
    double maxTime = 0;
    values.forEach((key, value){
      var time = value.last[0];
      if (time > maxTime) {
        maxTime = time;
      }
    });
    return maxTime;
  }

  void addValue(String label, double value) {
    // deal with the special case tha this is an altitude record.
    if (label == "A") {
      if (groundLevel == double.infinity) {
        groundLevel = value;
      }
      if (value > maxAltitude) {
        maxAltitude = value;
      }
      value -= groundLevel;
    }

    // Add the value to the record for this sample, and all the next unrecorded samples indicated by the sampleRate.
    final sampleRate = settings[records[label]!.setting]!.value;
    values[label] ??= [];
    values[label]!.add([records[label]!.time, value]);
    records[label]!.time += tickDuration * sampleRate;
  }

  Recording(Uint8List buffer) {
    var index = 0;
    var endOfBuffer = false;

    // reset record times
    records.forEach((k,v)=>(v.time = 0));

    String label = String.fromCharCode(buffer[index]);
    
    // walk through buffer adding values to correct records based on labels
    while(!endOfBuffer && records.containsKey(label)) {
      final record = records[label]!;
      final unit = units[record.unit]!;
      final start = index + 1;
      final end = index + 1 + record.length;
      final value = swapBytes(buffer.sublist(start, end)) / unit.slope + unit.offset;
      addValue(label, value); 
      index += record.length + 1;
      if (index < buffer.length) {
        label = String.fromCharCode(buffer[index]);
      } else {
        endOfBuffer = true;
      }
    } 
  }

  // Earlier attempts tried to use a single string for the whole CSV file.
  // This was very slow as strings are immutable so needed to be copied for every
  // character addition. As the string gets longer this gets slower and slower.
  List<String> getCSV() {
    List<String> columns = values.keys.toList();
    Map<String, int> timeKeeper = {};
    var shortestTick = 0xffffffff;
    var longestList = "";
    
    List<String> csv = [];
    String line = "Time";

    // Generate header row and initalize
    for (var label in columns) {
      timeKeeper[label] = 0;
      var sampleRate = settings[records[label]!.setting]!.value;
      if (sampleRate < shortestTick) {
        shortestTick = sampleRate;
        longestList = label;
      }
      line += ", ${records[label]!.title}";
    }
    csv.add(line);

    // Loop through all time in smallest tick increment. For each time build a row and
    // keep track of which index applies for the current time for each record.
    for (int tick = 0; tick < values[longestList]!.length; tick++) {
      var time = tick * shortestTick * tickDuration;
      line = time.toStringAsFixed(3);
      for (var label in columns) {
        var data = values[label]!;
        var dataIndex = timeKeeper[label]!;
        if (time > data[dataIndex][0] && dataIndex + 1 < data.length) {
          timeKeeper[label] = dataIndex + 1;
        }
        double value = data[dataIndex][1];
        line += ", ${value.toStringAsFixed(records[label]!.precision)}";
      }
      csv.add(line);
    }

    return csv;
  }
}