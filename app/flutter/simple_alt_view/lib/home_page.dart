import 'package:flutter/material.dart';
import 'package:simple_alt_view/altimeter.dart';
import 'package:simple_alt_view/graph_page.dart';
import 'package:simple_alt_view/setting_page.dart';


class HomePage extends StatelessWidget {
  final GlobalKey<GraphPageState> _key = GlobalKey<GraphPageState>(); 
  final String title;
  final altimeter = Altimeter();
  
  HomePage({super.key, required this.title});

  void connectAltimeter() {
    final altimeter = Altimeter();
    if(altimeter.findAltimeter()) {
      altimeter.sync();
    } else {
      altimeter.sync(filename:"621e7440.dump");
    }

    altimeter.recordingList.clear();
    altimeter.parseData();
    _key.currentState?.refreshLogList();
  }

  @override
  Widget build(BuildContext context) {
    return DefaultTabController(
        length: 2, 
        child: Scaffold(
          appBar: AppBar(
            bottom: const TabBar(
              indicatorSize: TabBarIndicatorSize.tab,            
              tabs: [
                Tab(icon: Icon(Icons.show_chart)),
                Tab(icon: Icon(Icons.settings)),
              ],
            ),
            title: Text(title),
            actions: <Widget> [
              IconButton(
              onPressed: connectAltimeter, 
              icon: const Icon(Icons.cable_rounded), 
            ),
        ]
          ),
          body: TabBarView(
            children: [
              GraphPage(key: _key),
              const SettingPage(),
            ],
          ),
        ),
      );
  }
}