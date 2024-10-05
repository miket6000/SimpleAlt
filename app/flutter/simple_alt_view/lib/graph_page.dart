import 'dart:io';
import 'dart:ui' as ui;
import 'dart:typed_data';
import 'package:flutter/rendering.dart';
import 'package:flutter/material.dart';
import 'package:simple_alt_view/altimeter.dart';
import 'package:simple_alt_view/altitude_chart.dart';
import 'package:simple_alt_view/recording.dart';

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
  const GraphPage({super.key});
  @override
  State<GraphPage> createState() => GraphPageState();
}

class GraphPageState extends State<GraphPage> with AutomaticKeepAliveClientMixin {
  //final GlobalKey<_AltitudeChartState> _key = GlobalKey();
  final altimeter = Altimeter();
  Recording? selectedRecording;

  @override
  bool get wantKeepAlive => true;

  void refreshLogList() {
    selectedRecording = null;
    if (altimeter.recordingList.isNotEmpty) {
      selectedRecording = altimeter.recordingList.last;
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
    super.build(context);
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
                recordingDropdown,
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

