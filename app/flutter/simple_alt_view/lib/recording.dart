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
  final int column;     //CSV column (0 is always time)
  double time = 0;
  bool plot;            //Do or donot show in graphs
  Color colour;         //Colour of graph line and axis text
  Record({required this.title, required this.setting, required this.length, required this.unit, required this.precision, required this.column, this.plot=true, this.colour=Colors.blue});
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
  "A": Record(title:"Altitude",    setting:"a", length:4, unit:"m",   precision:1, column:1, colour:Colors.blue,    plot:true),
  "P": Record(title:"Pressure",    setting:"p", length:4, unit:"hPa", precision:2, column:2, colour:Colors.orange,  plot:false),
  "T": Record(title:"Temperature", setting:"t", length:2, unit:"°C",  precision:1, column:3, colour:Colors.red,     plot:true),
  "V": Record(title:"Voltage",     setting:"v", length:2, unit:"V",   precision:2, column:4, colour:Colors.purple,  plot:false),
  "S": Record(title:"Status",      setting:"s", length:1, unit:"-",   precision:0, column:5, colour:Colors.green,   plot:false),
};

int swapBytes(Uint8List bytes) {
  int value = 0;
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

extension X<T> on List<List<double>> {
  double lastBefore(double n) {
    return where((e)=>(e[0] <= n)).toList().last[0];   
  }
}

class Recording {
  int index = 0;
  double maxAltitude = 0;
  double groundLevel = double.infinity;

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
    var sampleRate = settings[records[label]!.setting]!.value;
    values[label] ??= [];
    values[label]!.add([records[label]!.time, value]);
    records[label]!.time += tickDuration * sampleRate;
  }

  Recording(Uint8List buffer) {
    int index = 0;
    bool endOfBuffer = false;

    // reset record times
    records.forEach((k,v)=>(v.time = 0));

    String label = String.fromCharCode(buffer[index]);
    while(!endOfBuffer && records.containsKey(label)) {
      Unit unit = units[records[label]!.unit]!;
      double value = swapBytes(buffer.sublist(index + 1, index + 1 + records[label]!.length)) / unit.slope + unit.offset;
      addValue(label, value);
      index += records[label]!.length + 1;
      if (index < buffer.length) {
        label = String.fromCharCode(buffer[index]);
      } else {
        endOfBuffer = true;
      }
    } 
  }

  // VERY inefficient. Basically unworkably so. 
  // Also not very good as not all values are changed at the same time. I probably need to do like
  // I used to and create the data a row at a time, ideally only on data changes. 
  // Fundamentally I can have records that are hundreds of thousands of entries and millions of 
  // ticks long. This just creates problems.

  // Alternative approach...
  // use a variable to track when each list will next change. Walk though time and if a variable 
  // (or vairables) are changing, then create a row. It will mean that rows won't be a fixed time
  // step, but does that matter?

  // Or, just give up entirely.
  List<String> getCSV() {
    List<String> sortedColumns = values.keys.toList()..sort((a, b) => records[a]!.column.compareTo(records[b]!.column));
    Map<String, int> timeKeeper = {};
    var shortestTick = 0xffffffff;
    var longestList = "";
    
    List<String> csv = [];
    String line = "Time";

    for (var label in sortedColumns) {
      timeKeeper[label] = 0;
      var sampleRate = settings[records[label]!.setting]!.value;
      if (sampleRate < shortestTick) {
        shortestTick = sampleRate;
        longestList = label;
      }
      line += ", ${records[label]!.title}";
    }

    csv.add(line);

    for (int tick = 0; tick < values[longestList]!.length; tick++) {
      var time = tick * shortestTick * tickDuration;
      line = time.toStringAsFixed(3);
      for (var label in sortedColumns) {
        if (time > values[label]![timeKeeper[label]!][0] && 
            timeKeeper[label]! < values[label]!.length - 1) {
          timeKeeper[label] = timeKeeper[label]! + 1;
        }
        double value = values[label]![timeKeeper[label]!][1];
        line += ", ${value.toStringAsFixed(records[label]!.precision)}";
      }
      csv.add(line);
    }

    return csv;
  }
}