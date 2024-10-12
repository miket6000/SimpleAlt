import 'package:flutter/material.dart';
import 'package:simple_alt_view/altimeter.dart';
import 'package:simple_alt_view/graph_page.dart';
import 'package:simple_alt_view/setting_page.dart';
import 'package:simple_alt_view/main.dart';

class HomePage extends StatelessWidget {
  final String title;
  final altimeter = Altimeter();
  
  HomePage({super.key, required this.title});

  void connectAltimeter() {
    int uid = altimeter.connect();
    if(uid != 0) {
      altimeter.sync();
      scaffoldKey.currentState!.showSnackBar(
        SnackBar(
          content: Text("Successfully connected to ${altimeter.uid.toRadixString(16)}")
        )
      );
    } else {
      altimeter.sync(filename:"621e7440.dump");
      scaffoldKey.currentState!.showSnackBar(
        SnackBar(
          content: Text("Could not connect to altimeter, instead loaded ${altimeter.uid.toRadixString(16)} from file")
        )
      );
    }

    altimeter.parseData();
    graphPageKey.currentState?.refreshDropList();
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
            GraphPage(key: graphPageKey, altimeter: altimeter),
            SettingPage(altimeter: altimeter),
          ],
        ),
      ),
    );
  }
}