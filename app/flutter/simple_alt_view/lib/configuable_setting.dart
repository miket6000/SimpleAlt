import 'package:flutter/material.dart';
import 'recording.dart';

class ConfigurableSetting extends StatefulWidget {
  final Setting setting;
  const ConfigurableSetting({super.key, required this.setting});

  @override
  State<ConfigurableSetting> createState() => _ConfigurableSettingState();
}

class _ConfigurableSettingState extends State<ConfigurableSetting> {
  late TextEditingController _controller;
  late TextField valueTextField;
  int initialValue = 0;
  bool enabled = true;
  
  @override
  void initState() {
    initialValue = widget.setting.value;
    enabled = widget.setting.value > 0;
    _controller = TextEditingController(text: initialValue.toString())
      ..addListener((){
        widget.setting.value = int.tryParse(_controller.text) ?? 0;
        setState((){});
      });
    valueTextField = TextField(controller:_controller);
    super.initState();
  }
  
  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.start,
      children: [
        Switch(
          value: enabled, 
          onChanged: (bool value) {
            if (value) {
              if (int.tryParse(_controller.text) != null) {
                enabled = true;
                widget.setting.value = int.parse(_controller.text);
              }
            } else {
              widget.setting.value = 0;
              enabled = false;
            }
            setState((){});
          }
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
          child: valueTextField, 
        ),
      ]
    );
  }
}

