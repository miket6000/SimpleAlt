import 'dart:developer';
import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:simple_alt_view/altimeter.dart';

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

void onSave(){
  log("save button was pressed");
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;
  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  final altimeter = Altimeter();
  List<Recording> recordingList = [];
  List<String> recordingStrings = [];
  late Recording dropdownValue;
  late List<FlSpot> points;


  @override
  void initState() {
    super.initState();
    points = <FlSpot>[];
    refreshLogList();
  }

  void refreshLogList() {
    if (altimeter.findAltimeter()) {
      recordingList = altimeter.getRecordingList();
      dropdownValue = recordingList.first;
    } else {
      dropdownValue = Recording(index: 0, startAddress: 0, endAddress: 0);
    }
    setState(() { /* required to force update of dropdownMenu*/ });
  }

  @override
  Widget build(BuildContext context) {
    LineChart altitudeChart = LineChart(
      key: ValueKey(dropdownValue), //changing dropdownValue changes the key forcing an update on the next call to setState();
      LineChartData(
        //borderData: FlBorderData(border: const Border(bottom: BorderSide(), left: BorderSide())), 
        lineBarsData: [
          LineChartBarData(spots: points, dotData: const FlDotData(show: false,),)
        ],
        titlesData: const FlTitlesData(
          topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
          rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
//          bottomTitles: AxisTitles(sideTitles: SideTitles(showTitles: true, interval: 20)),
        ),
        maxX: (points.length > 1 ? ((points.reduce((cur, next) => cur.x > next.x ? cur: next)).x / 10).ceil() * 10.0 : 0),
        minY: (points.length > 1 ? ((points.reduce((cur, next) => cur.y < next.y ? cur: next)).y / 10).floor() * 10.0 : 0),
        maxY: (points.length > 1 ? ((points.reduce((cur, next) => cur.y > next.y ? cur: next)).y / 10).ceil() * 10.0 : 0),
        
      ),
    );

    DropdownMenu recordingDropdown = DropdownMenu<Recording>(
      initialSelection: dropdownValue,
      width: 400, 
      onSelected: (Recording? value) {
        // user selected a recording...
        altimeter.fetchRecording(value!.index); // get data for selected recording
        points = value.altitude.asMap().entries.map((e) => FlSpot(e.key.toDouble()*0.02, e.value)).toList(); //update graph data
        setState(() {
          dropdownValue = value; //force update by setting new value
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
          IconButton(onPressed: refreshLogList, icon: const Icon(Icons.cable_rounded))
        ]
      ),
      body:Column(
        mainAxisAlignment: MainAxisAlignment.start,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: <Widget>[
          Expanded(             
            child: Padding (
              padding: const EdgeInsets.all(20),
              child: altitudeChart,
            )
          ),
          Padding(
            padding: const EdgeInsets.all(20),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                recordingDropdown,
                const ElevatedButton(
                  onPressed: onSave, 
                  child: Text("Save"),
                )
              ]
            )
          )
        ],
      ), 
    );
  }
}