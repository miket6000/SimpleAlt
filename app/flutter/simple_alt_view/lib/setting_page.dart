import 'package:flutter/material.dart';
import 'package:simple_alt_view/recording.dart';
import 'recording_display_setting.dart';

class SettingPage extends StatefulWidget {
  const SettingPage({super.key});
  @override
  State<SettingPage> createState() => _SettingPageState();
}

class _SettingPageState extends State<SettingPage> {
  //final GlobalKey<_AltitudeChartState> _key = GlobalKey();
  Recording? selectedRecording;

  @override
  Widget build(BuildContext context) {
    return Column(
        mainAxisAlignment: MainAxisAlignment.start,
        crossAxisAlignment: CrossAxisAlignment.end,
        children: [
          for (var record in records.keys) 
            RecordDisplaySetting(record: records[record]!),
        ],
      );
  }
}

