import 'dart:io';

import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:simple_alt_view/altimeter.dart';
import 'package:simple_alt_view/altitude_chart.dart';
import 'package:simple_alt_view/recording.dart';

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;
  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  //final GlobalKey<_AltitudeChartState> _key = GlobalKey();
  final altimeter = Altimeter();
  List<String> recordingStrings = [];
  Recording? selectedRecording;
  List<List<FlSpot>> points = [];
  bool loading = false;

  @override
  void initState() {
    super.initState();
  }

  void refreshLogList() {
    if(altimeter.findAltimeter()) {
      altimeter.sync();
    } else {
      altimeter.sync(filename:"621e7440.dump");
    }

    altimeter.recordingList.clear();
    selectedRecording = null;
    altimeter.parseData();
    
    if (altimeter.recordingList.isNotEmpty) {
      selectedRecording = altimeter.recordingList[0];
    }
    setState(() { /* required to force update of dropdownMenu*/ });
  }

  void onSave() {
    final scaffold = ScaffoldMessenger.of(context);

    if (selectedRecording != null) {
      var now = DateTime.now();
      var month = now.month.toString().padLeft(2, '0');
      var day = now.day.toString().padLeft(2, '0');
      var hour = now.hour.toString().padLeft(2, '0');
      var minute = now.minute.toString().padLeft(2, '0');
      var second = now.second.toString().padLeft(2, '0');
      var datetime = "${now.year}$month${day}_$hour$minute$second"; 
      
      String filename = "SimpleAlt-${altimeter.uid.toRadixString(16)}-${altimeter.recordingList.indexOf(selectedRecording!)}-$datetime.csv";
      String csv = selectedRecording!.getCSV();
      File file = File(filename);
      file.writeAsStringSync(csv);

      scaffold.showSnackBar(
        SnackBar(
          content: Text('File saved as $filename'),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    DropdownMenu recordingDropdown = DropdownMenu<Recording>(
      initialSelection: selectedRecording,
      width: 400, 
      onSelected: (Recording? recording) {
        // user selected a recording...
        selectedRecording = recording!;
        altimeter.recordingList[recording.index]; // get data for selected recording
        setState(() {});
      },
      dropdownMenuEntries: altimeter.recordingList.map<DropdownMenuEntry<Recording>>((Recording recording) {
        return DropdownMenuEntry<Recording>(value: recording, label: 'Recording ${altimeter.recordingList.indexOf(recording)}, duration ${recording.getDuration().toStringAsFixed(1)}s');
      }).toList(),
    );

    return Scaffold(
      appBar: AppBar(
        //backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        backgroundColor: Theme.of(context).primaryColor,
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
              child: AltitudeChart(recording: selectedRecording),
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

