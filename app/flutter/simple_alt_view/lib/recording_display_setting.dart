import 'package:flutter/material.dart';
import 'package:simple_alt_view/graph_page.dart';
import 'package:simple_alt_view/recording.dart';

class RecordDisplaySetting extends StatefulWidget {
  final Record record;
  const RecordDisplaySetting({super.key, required this.record});

  @override
  State<RecordDisplaySetting> createState() => _RecordDisplaySettingState();
}

class _RecordDisplaySettingState extends State<RecordDisplaySetting> {
  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.start,
      children: [
        Switch(
          value: widget.record.plot, 
          onChanged: (bool value) {
            graphPageKey.currentState!.update();
            setState(() => widget.record.plot = value);
          },
        ), 
        Padding (
          padding: const EdgeInsets.only(left:10, right:10),
          child: Container(
            width: 30.0,
            height: 30.0,
            decoration: BoxDecoration(
              color: widget.record.colour,
              shape: BoxShape.circle,
            ),
          ),
        ),
        Text(widget.record.title),
      ]
    );
  }
}
