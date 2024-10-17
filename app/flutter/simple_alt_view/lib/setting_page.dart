import 'package:flutter/material.dart';
import 'package:simple_alt_view/recording.dart';
import 'package:simple_alt_view/altimeter.dart';
import 'recording_display_setting.dart';
import 'configuable_setting.dart';

final GlobalKey<SettingPageState> settingPageKey = GlobalKey<SettingPageState>(); 

class SettingPage extends StatefulWidget {
  final Altimeter altimeter;
  const SettingPage({super.key, required this.altimeter});
  @override
  State<SettingPage> createState() => SettingPageState();
}

class SettingPageState extends State<SettingPage> {
  Recording? selectedRecording;
  List<ConfigurableSetting> settingWidgets = [];
  List<RecordDisplaySetting> displayWidgets = [];
  Map<String, int> initValues = {};
  double flashUtilization = 0;

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

  void erase() {
    widget.altimeter.sendCommand("ERASE");
    setState(() {});
  }

  void factoryReset() {
    widget.altimeter.sendCommand("RESET");
    setState(() {});
  }

  void update() {
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

    flashUtilization = widget.altimeter.flashUtilization;
    
    super.initState();    
  }

  @override
  Widget build(BuildContext context) {
    bool wideScreen = MediaQuery.sizeOf(context).width > 1000;

    return 
      SingleChildScrollView(child:
      Padding(
        padding: const EdgeInsets.all(8),
        child: Column(
        children: [
        Flex(
          direction: wideScreen ? Axis.horizontal : Axis.vertical,
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.spaceAround,
          children: [
              Container(
                constraints: const BoxConstraints(maxWidth:500, maxHeight: 300), 
                child: Column(
                  children: [
                    const Text("Plot Settings"),
                    ...displayWidgets,
                  ]
                ),
              ),
              Container(
                constraints: const BoxConstraints(maxWidth:500, maxHeight: 400),
                child: Column( 
                  children:[
                    const Text("Altimeter Settings"),
                    ...settingWidgets,
                  ]
                ),
              ),
          ]   
        ),
        Flex(
          direction: wideScreen ? Axis.horizontal : Axis.vertical,
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          crossAxisAlignment: CrossAxisAlignment.end,
          children:[
            Row(
              //mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              //crossAxisAlignment: CrossAxisAlignment.end,
              children:[
                Text("${((1 - flashUtilization) * 100).toStringAsFixed(2)}% Flash Remaining"),
                const SizedBox(width:100),
                ElevatedButton(
                  onPressed: erase, 
                  child: const Text("Erase Flash"),
                ),
              ]
            ),
            Row(
              //mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              //crossAxisAlignment: CrossAxisAlignment.end,
              children:[
                ElevatedButton(
                  onPressed: factoryReset, 
                  child: const Text("Factory Reset"),
                ),
                const SizedBox(width:100),
                ElevatedButton(
                  onPressed: updateSettings, 
                  child: const Text("Upload Settings"),
                ),
              ]
            ),
          ],
        ),
        ]
      )
      )
    );
  }

}

