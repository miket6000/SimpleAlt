import 'package:flutter/material.dart';
import 'recording.dart';

class ConfigurableSetting extends StatefulWidget {
  final Setting setting;
  const ConfigurableSetting({super.key, required this.setting});

  @override
  State<ConfigurableSetting> createState() => _ConfigurableSettingState();
}

class _ConfigurableSettingState extends State<ConfigurableSetting> {
  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.start,
      children: [
        Switch(
          value: widget.setting.value > 0, 
          onChanged: (bool value) {
            setState(() => widget.setting.value = 0);
          },
        ), 
        Padding(
          padding: const EdgeInsets.only(left:10, right:10), 
          child: SizedBox(
            width:200,
            child: Text(widget.setting.title),
          ),
        ),
        SizedBox(
          width:200, 
          child: TextFormField(initialValue: widget.setting.value.toString(),),
        ),
        //const Spacer(),
      ]
    );
  }
}

