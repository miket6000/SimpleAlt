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
    final scaffold = ScaffoldMessenger.of(context);

    String filename = selectedRecording.saveAsCSV();

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
        selectedRecording = value!;
        altimeter.fetchRecording(value.index); // get data for selected recording
        points = value.altitude.map((sample) => FlSpot(sample.time, sample.altitude)).toList(); //update graph data
        setState(() {});
      },
      dropdownMenuEntries: recordingList.map<DropdownMenuEntry<Recording>>((Recording value) {
        return DropdownMenuEntry<Recording>(value: value, label: 'Recording ${value.index}, duration ${value.getDuration().toStringAsFixed(1)}s');
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

