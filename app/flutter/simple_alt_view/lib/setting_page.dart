import 'package:flutter/material.dart';
import 'package:simple_alt_view/recording.dart';
import 'package:simple_alt_view/altimeter.dart';
import 'recording_display_setting.dart';
import 'configuable_setting.dart';

class SettingPage extends StatefulWidget {
  final Altimeter altimeter;
  const SettingPage({super.key, required this.altimeter});
  @override
  State<SettingPage> createState() => _SettingPageState();
}

class _SettingPageState extends State<SettingPage> {
  Recording? selectedRecording;
  List<ConfigurableSetting> settingWidgets = [];
  List<RecordDisplaySetting> displayWidgets = [];
  Map<String, int> initValues = {};

  void updateSettings() {
    final scaffold = ScaffoldMessenger.of(context);
    bool success = true;
    for (var setting in initValues.keys) {
      if (settings[setting]!.value != initValues[setting]!) {
        if (!widget.altimeter.saveSetting(setting)) {
          success = false;
        }
      }
    }
    if (success) {
      scaffold.showSnackBar(
        const SnackBar(
          content: Text('Settings successfully updated.'),
        ),
      );        
    } else {
      scaffold.showSnackBar(
        const SnackBar(
          content: Text('Error: Could not save settings, is the altimeter connected and turned on?'),
        ),
      );
    }
    setState(() {});
  }

  @override
  void initState() {
    for (var setting in settings.keys.where((e)=>settings[e]!.configurable)) {
      initValues[setting] = settings[setting]!.value;
      settingWidgets.add(ConfigurableSetting(setting: settings[setting]!));
    }

    for (var record in records.keys) {
      displayWidgets.add(RecordDisplaySetting(record: records[record]!));
    }
    
    super.initState();    
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.start,
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [
          const Text("Plot Settings"),
          ...displayWidgets,
          const Text("Altimeter Settings"),
          ...settingWidgets,
          Row(
            mainAxisAlignment: MainAxisAlignment.end,
            children:[
              ElevatedButton(
                onPressed: updateSettings, 
                child: const Text("Upload Settings"),
              )
            ],
          ),
        ],
      )
    );
  }
}

