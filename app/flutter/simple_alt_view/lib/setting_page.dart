import 'package:flutter/material.dart';
import 'package:simple_alt_view/recording.dart';
import 'recording_display_setting.dart';
import 'configuable_setting.dart';

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
    return Padding(
      padding: const EdgeInsets.all(8),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.start,
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [
          const Text("Plot Settings"),
          for (var record in records.keys) 
            RecordDisplaySetting(record: records[record]!),
          const Text("Altimeter Settings"),
          for (var setting in settings.keys.where((e)=>settings[e]!.configurable))
            ConfigurableSetting(setting: settings[setting]!),
        ],
      )
    );
  }
}

