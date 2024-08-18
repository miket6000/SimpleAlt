import 'dart:io';
import 'package:csv/csv.dart';

const int recordLength = 5;

class Sample {
  double time = 0;
  double altitude = 0;
  Sample({required this.time, required this.altitude});
}

class Recording {
  int index = 0;
  int startAddress = 0;
  int endAddress = 0;
  List<Sample> data = [];
  double groundLevel = 0;
  double sampleTime = 0.02;

  Recording({required this.index, required this.startAddress, required this.endAddress});
  
  double getDuration() => (endAddress - startAddress) * sampleTime / recordLength;
  bool hasData() => (data.isNotEmpty);
  List<Sample> get altitude => data.map((sample) => Sample(time: sample.time, altitude: sample.altitude - groundLevel)).toList();

  String saveAsCSV([String filename = '']) {    
    if (filename == '') {
      int suffix = 0;
      filename = "SimpleAlt_Flight_$index.csv";

      while(FileSystemEntity.typeSync(filename) != FileSystemEntityType.notFound) {
        suffix++;
        filename = "SimpleAlt_Flight_$index-$suffix.csv";
      }
    }

    final List<List<String>> list = [["Time", "Altitude"]] + data.map<List<String>>((sample) => [sample.time.toStringAsFixed(2), sample.altitude.toStringAsFixed(2)]).toList();
    final String csv = const ListToCsvConverter().convert(list);
  
    File file = File(filename);
    var writer = file.openWrite();
    writer.write(csv); 
    writer.close();
    return filename;
  }
}

