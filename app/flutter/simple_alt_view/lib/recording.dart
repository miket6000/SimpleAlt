const double tickDuration = 0.01;

class Setting {
  late final String title;
  int value = 0;
  Setting({required title, value});
}

class Unit {
  late final String title;
  late final double slope;
  late final double offset;
  Unit({required title, required slope, required offset});
}

class Record {
  late final String title;
  late final int length;
  late final String setting;
  late final String unit;
  late final int column;
  int value = 0;
  Record({required title, required setting, required length, required unit, required column, value});
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
  "v":    Unit(title:"Voltage",       slope:1000,         offset:0),
  "m":    Unit(title:"Altitude",      slope:100,          offset:0),
  "ft":   Unit(title:"Altitude",      slope:30.48,        offset:0),
  "hPa":  Unit(title:"Pressure",      slope:100,          offset:0),
  "psi":  Unit(title:"Pressure",      slope:6894.75729,   offset:0),
  "C":    Unit(title:"Temperature",   slope:100,          offset:0),
  "F":    Unit(title:"Temperature",   slope:180,          offset:32),
  "-":    Unit(title:"Status",        slope:1,            offset:0),
};

final Map<String, Record> records = {
  "A": Record(title:"Altitude",    setting:"a", length:4, unit:"m",   column:1),
  "P": Record(title:"Pressure",    setting:"p", length:4, unit:"hpa", column:2),
  "T": Record(title:"Temperature", setting:"t", length:2, unit:"C",   column:3),
  "V": Record(title:"Voltage",     setting:"v", length:2, unit:"V",   column:4),
  "S": Record(title:"Status",      setting:"s", length:1, unit:"-",   column:5),
};

int swapBytes(List<int> bytes) {
  if (bytes.length == 1) {
    return bytes.first;
  } 
  if (bytes.length == 2) {
    return bytes[1] << 8 + bytes[0];
  }
  if (bytes.length == 4) {
    return bytes[3] << 24 + bytes[2] << 16 + bytes[1] << 8 + bytes[0];
  }
  throw "Invalid number of bytes"; 
}

class Recording {
  double maxAltitude = 0;
  double groundLevel = double.infinity;
  double time = 0;
  List<int> sampleRates = [];
  late String highestSampledRecordType;
  int highestSampleRate = 0xffffffff;
  List<List> rows = [];
  List<double> currentRow = [0, 0, 0, 0, 0, 0];
  int getRowCount() => rows.length;
  double getDuration() => rows.length * (highestSampleRate / 100);
  
  void addValue(String label, double value) {
    currentRow[records[label]!.column] = value;
    if(label == highestSampledRecordType) {
      rows.add(currentRow);
      time += tickDuration;
      currentRow[0] = time;
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

  Recording(List<int> buffer) {
    int index = 0;
    String label = String.fromCharCode(buffer[index]);
    findHighestSampledRecordType();
    while(index < buffer.length && records.containsKey(label)) {
      Record record = records[label]!;
      Unit unit = units[record.unit]!;
      double value = swapBytes(buffer.sublist(index + 1, record.length)) / unit.slope + unit.offset;
      addValue(label, value);
      index += record.length + 1;
      label = String.fromCharCode(buffer[index]);
    } 
  }
}