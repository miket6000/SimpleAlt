import 'dart:typed_data';

const double tickDuration = 0.01;

class Setting {
  final String title;
  int value;
  Setting({required this.title, this.value = 0});
}

class Unit {
  final String title;
  final double slope;
  final double offset;
  Unit({required this.title, required this.slope, required this.offset});
}

class Record {
  final String title;
  final int length;
  final String setting;
  final String unit;
  final int precision;
  final int column ;
  Record({required this.title, required this.setting, required this.length, required this.unit, required this.precision, required this.column});
}

final Map<String, Setting> settings = {
  "a": Setting(title:"Altitude Sample Rate"),
  "p": Setting(title:"Pressure Sample Rate"),
  "t": Setting(title:"Temperature Sample Rate"),
  "v": Setting(title:"Voltage Sample Rate"),
  "s": Setting(title:"Status Sample Rate"), 
  "m": Setting(title:"Maximum Altitude"),
  "o": Setting(title:"Power Off Timeout"),
  "r": Setting(title:"Recording Start Address"),
};

final Map<String, Unit> units = {
  "V":    Unit(title:"Voltage",       slope:1000,         offset:0),
  "m":    Unit(title:"Altitude",      slope:100,          offset:0),
  "ft":   Unit(title:"Altitude",      slope:30.48,        offset:0),
  "hPa":  Unit(title:"Pressure",      slope:100,          offset:0),
  "psi":  Unit(title:"Pressure",      slope:6894.75729,   offset:0),
  "C":    Unit(title:"Temperature",   slope:100,          offset:0),
  "F":    Unit(title:"Temperature",   slope:180,          offset:32),
  "-":    Unit(title:"Raw",           slope:1,            offset:0),
};

final Map<String, Record> records = {
  "A": Record(title:"Altitude",    setting:"a", length:4, unit:"m",   precision:1, column:1),
  "P": Record(title:"Pressure",    setting:"p", length:4, unit:"hPa", precision:2, column:2),
  "T": Record(title:"Temperature", setting:"t", length:2, unit:"C",   precision:1, column:3),
  "V": Record(title:"Voltage",     setting:"v", length:2, unit:"V",   precision:2, column:4),
  "S": Record(title:"Status",      setting:"s", length:1, unit:"-",   precision:0, column:5),
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

class Recording {
  int index = 0;
  double maxAltitude = 0;
  double groundLevel = double.infinity;
  double time = 0;
  late String highestSampledRecordType;
  int highestSampleRate = 0xffffffff;
  List<List> rows = [];
  List<double> currentRow = [0, 0, 0, 0, 0, 0];
  int getRowCount() => rows.length;
  double getDuration() => rows.length * (highestSampleRate / 100);
  List<String> sortedColumns = records.keys.toList()..sort((a, b) => records[a]!.column.compareTo(records[b]!.column));

  List getAltitude() => [...rows.map((e) => [e[0], e.elementAt(records["A"]!.column)])];
  List getTemperature() => [...rows.map((e) => [e[0], e.elementAt(records["T"]!.column)])];
  List getPressure() => [...rows.map((e) => [e[0], e.elementAt(records["P"]!.column)])];
  List getVoltage() => [...rows.map((e) => [e[0], e.elementAt(records["V"]!.column)])];
  List getStatus() => [...rows.map((e) => [e[0], e.elementAt(records["S"]!.column)])];
 
  void addValue(String label, double value) {
    currentRow[records[label]!.column] = value;
    if(records[label]!.setting == highestSampledRecordType) {
      // Skip the first sample as it comes from the data inialization block in the altimeter
      if (time > 0) {
        currentRow[0] = time;
        rows.add(List.from(currentRow));
      }
      time += tickDuration * highestSampleRate;
    }
    if (label == "A") {
      if (groundLevel == double.infinity) {
        groundLevel = value;
      }
      if (value > maxAltitude) {
        maxAltitude = value;
      }
    }
  }

  findHighestSampledRecordType() {
    records.forEach((key, value) {
      var sampleRate = settings[value.setting]!.value;
      if (sampleRate != 0 && sampleRate < highestSampleRate) {
        highestSampledRecordType = value.setting;
        highestSampleRate = sampleRate;
      }
    });   
  }

  Recording(Uint8List buffer) {
    int index = 0;
    bool endOfBuffer = false;
    String label = String.fromCharCode(buffer[index]);
    findHighestSampledRecordType();
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

  String getCSV() {
    String csv = "Time";

    for (var label in sortedColumns) {
      csv += ", ${records[label]!.title}";
    }

    for (var row in rows) {
      csv += "\n${row[0].toStringAsFixed(2)}";
      for (var label in sortedColumns) {
        double value = row[records[label]!.column];
        csv += ", ${value.toStringAsFixed(records[label]!.precision)}";
      }
    }

    return csv;
  }
}