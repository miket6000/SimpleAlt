import 'dart:developer';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:simple_alt_view/altimeter.dart';
import 'package:simple_alt_view/altitude_chart.dart';
import 'package:csv/csv.dart';
import 'package:file_saver/file_saver.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of the application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'SimpleAlt Viewer',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple, brightness: Brightness.dark),
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'SimpleAlt Viewer'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;
  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  //final GlobalKey<_AltitudeChartState> _key = GlobalKey();
  final altimeter = Altimeter();
  List<Recording> recordingList = [];
  List<String> recordingStrings = [];
  late Recording selectedRecording;
  late List<FlSpot> points;
  bool loading = false;

  @override
  void initState() {
    super.initState();
    points = <FlSpot>[];
    refreshLogList();
  }

  void refreshLogList() {
    if (altimeter.findAltimeter()) {
      recordingList = altimeter.getRecordingList();
      selectedRecording = recordingList.first;
    } else {
      selectedRecording = Recording(index: 0, startAddress: 0, endAddress: 0);
    }
    setState(() { /* required to force update of dropdownMenu*/ });
  }

  void onSave() {
    int recordingNumber = recordingList.indexOf(selectedRecording);
    int suffix = 0;
    String filename = "SimpleAlt_Flight_$recordingNumber.csv";
    while(FileSystemEntity.typeSync(filename) != FileSystemEntityType.notFound) {
      suffix++;
      filename = "SimpleAlt_Flight_$recordingNumber - $suffix.csv";
    }

    final List<List<double>> list = points.map((value) => [value.x, value.y]).toList();
    final String csv = const ListToCsvConverter().convert(list);
    final scaffold = ScaffoldMessenger.of(context);
  
    File file = File(filename);
    var writer = file.openWrite();
    writer.write(csv); 
    writer.close();
    scaffold.showSnackBar(
      SnackBar(
        content: Text('File saved as $filename'),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    DropdownMenu recordingDropdown = DropdownMenu<Recording>(
      initialSelection: selectedRecording,
      width: 400, 
      onSelected: (Recording? value) {
        // user selected a recording...
        altimeter.fetchRecording(value!.index); // get data for selected recording
        points = value.altitude.asMap().entries.map((e) => FlSpot(e.key.toDouble()*0.02, e.value)).toList(); //update graph data
        setState(() {
          selectedRecording = value; //force update by setting new value
        });
      },
      dropdownMenuEntries: recordingList.map<DropdownMenuEntry<Recording>>((Recording value) {
        return DropdownMenuEntry<Recording>(value: value, label: 'Recording ${value.index}, duration ${value.getDuration().toStringAsFixed(1)}s');
      }).toList(),
    );

    return Scaffold(
      appBar: AppBar(
        //backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        backgroundColor: Colors.black,
        title: Text(widget.title), 
        actions: <Widget> [
          IconButton(
            onPressed: refreshLogList, 
            icon: const Icon(Icons.cable_rounded), 
          ),
        ]
      ),
      body:Column(
        mainAxisAlignment: MainAxisAlignment.start,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: <Widget>[
          Expanded(
            child: Padding (
              padding: const EdgeInsets.all(20),
              child: AltitudeChart(points: points),
            )
          ),

          Padding(
            padding: const EdgeInsets.only(left:20, right:20, bottom:20, top:0),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                recordingDropdown,
                ElevatedButton(
                  onPressed: onSave, 
                  child: const Text("Save"),
                )
              ]
            )
          )
        ],
      ),
    );
  }
}
