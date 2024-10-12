import 'dart:io';
import 'dart:ui' as ui;
import 'dart:typed_data';
import 'package:flutter/rendering.dart';
import 'package:flutter/material.dart';
import 'package:simple_alt_view/altimeter.dart';
import 'package:simple_alt_view/altitude_chart.dart';
import 'package:simple_alt_view/recording.dart';

final GlobalKey<GraphPageState> graphPageKey = GlobalKey<GraphPageState>(); 

Future screenshot() async {
  RenderRepaintBoundary boundary = screenshotKey.currentContext!.findRenderObject() as RenderRepaintBoundary;
  var image = await boundary.toImage();
  ByteData? byteData = await image.toByteData(format: ui.ImageByteFormat.png);
  Uint8List pngBytes = byteData!.buffer.asUint8List();
  const directory = '.';
  File imgFile = File('$directory/ScreenShot.png');
  imgFile.writeAsBytes(pngBytes);
}

class GraphPage extends StatefulWidget {
  final Altimeter altimeter;
  const GraphPage({super.key, required this.altimeter});
  @override
  State<GraphPage> createState() => GraphPageState();
}

class GraphPageState extends State<GraphPage> with AutomaticKeepAliveClientMixin {
  //final GlobalKey<_AltitudeChartState> _key = GlobalKey();
  Recording? selectedRecording;
  List<Recording> recordingList = [];

  @override
  bool get wantKeepAlive => true;

  void refreshDropList() {
    selectedRecording = null;
    if (widget.altimeter.recordingList.isNotEmpty) {
      selectedRecording = widget.altimeter.recordingList.last;
    }
    setState(() { /* update dropdown menu */ });
  }

  // external facing function to force state update. Not pretty, but the easiest way to allow the settings page to update the graph.
  void update() {
    setState(() {});
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
      
      String uidStr = widget.altimeter.uid.toRadixString(16);
      String indexStr = widget.altimeter.recordingList.indexOf(selectedRecording!).toString();
      String filename = "SimpleAlt-$uidStr-$indexStr [$datetime].csv";
      String csv = selectedRecording!.getCSV().join("\n");
      File file = File(filename);
      file.writeAsStringSync(csv);

      scaffold.showSnackBar(
        SnackBar(
          content: Text('File saved as $filename'),
        ),
      );
    }
  }

  DropdownMenu<Recording> recordingDropdown() {
    return DropdownMenu<Recording> (
      initialSelection: selectedRecording,
      width: 400, 
      onSelected: (Recording? recording) {
        // user selected a recording...
        selectedRecording = recording!;
        setState(() {});  
      },
      dropdownMenuEntries: recordingList.map<DropdownMenuEntry<Recording>>((Recording recording) {
        String indexStr = recordingList.indexOf(recording).toString(); 
        String durationStr = recording.getDuration().toStringAsFixed(1); 
        return DropdownMenuEntry<Recording>(value: recording, label: "Recording $indexStr, duration ${durationStr}s");
      }).toList(),
    );
  }

  @override
  Widget build(BuildContext context) {
    super.build(context);
    recordingList = widget.altimeter.recordingList;

    return Column(
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
                recordingDropdown(),
                const Spacer(),
                ElevatedButton(
                  onPressed: onSave, 
                  child: const Text("Export as CSV"),
                ),
                const ElevatedButton(
                  onPressed: screenshot, 
                  child: Text("Export as PNG"),
                ),
              ]
            )
          )
        ],
      );
  }
}

