import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'recording.dart';

class AltitudeChart extends StatefulWidget {
  final Recording? recording;
  const AltitudeChart({super.key, this.recording});

  @override
  State<AltitudeChart> createState() => _AltitudeChartState();
}

class YAxis {
  Recording recording;
  String label;
  double minY = 0;
  double maxY = 0;
  double scalar = 0;
  int precision = 2;
  Color colour = Colors.blue;
  List<FlSpot> spots = [];

  double getY(double y) => y * scalar + minY;
  
  YAxis(this.recording, this.label) {
    colour = records[label]!.colour;

    minY = [...recording.rows.map((e) => e.elementAt(records[label]!.column))].reduce((a, b)=>(a < b ? a : b)); 
    maxY = [...recording.rows.map((e) => e.elementAt(records[label]!.column))].reduce((a, b)=>(a > b ? a : b)); 
    if (minY == maxY) {
      scalar = 1;
    } else {
      scalar = (maxY - minY);
    }

    spots = [...recording.rows.map((e) => FlSpot(e[0], (e.elementAt(records[label]!.column) - minY)/ scalar))];
  }
}

class _AltitudeChartState extends State<AltitudeChart> {
  Recording? recording; 

  @override
  void initState() {
    super.initState();
  }
  
  Widget customYAxisLabel({required String value, required Color colour}) {
    return Expanded(
      child: Text(
        value,
        style: TextStyle(fontSize: 12, color: colour),
      )
    );
  }
          
  Widget customYAxis(YAxis axis) {
    List<double> labels = [];
    double step = (axis.maxY - axis.minY) / 20;
    for (int i = 0; i <= 20; i++) {
      labels.add(axis.maxY - step * i);
    }
    return Padding(
      padding:const EdgeInsets.symmetric(vertical: 0, horizontal: 10),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          for (var label in labels)
          customYAxisLabel(value: label.toStringAsFixed(axis.precision), colour:axis.colour),
        ],
      )
    );
  }

  @override
  Widget build(BuildContext context) {
    List<YAxis> axis = [];
    if (widget.recording != null) {
      for (var record in records.keys) {
        if (records[record]!.plot) {
          axis.add(YAxis(widget.recording!, record));
        }
      }
    }

    return Row(
      children:[
        for (var ax in axis) customYAxis(ax),
        Expanded(
          child: Padding(
            padding: const EdgeInsets.only(left:20),
              child: LineChart(
                LineChartData (
                //borderData: FlBorderData(border: const Border(bottom: BorderSide(), left: BorderSide())), 
                lineBarsData: [
                  for (var ax in axis)
                    LineChartBarData(spots: ax.spots, dotData: const FlDotData(show: false,), color:ax.colour),
                ],
                titlesData: const FlTitlesData(
                  topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                  rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                  leftTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)), 
                  //bottomTitles: AxisTitles(sideTitles: SideTitles(showTitles: true, interval: 20)),
                ),
                //maxX: (dataLength > 1 ? ((points[0].reduce((cur, next) => cur.x > next.x ? cur: next)).x / 10).ceil() * 10.0 : 0),
                //minY: 0, //(dataLength > 1 ? ((points[0].reduce((cur, next) => cur.y < next.y ? cur: next)).y / 10).floor() * 10.0 : 0),
                //maxY: 1.0, //(dataLength > 1 ? ((points[0].reduce((cur, next) => cur.y > next.y ? cur: next)).y / 10).ceil() * 10.0 : 0),
                lineTouchData: LineTouchData(
                  touchTooltipData: LineTouchTooltipData(
                    fitInsideHorizontally: true,
                    fitInsideVertically: true,
                    getTooltipItems: (touchedSpots) {
                      return touchedSpots.map((LineBarSpot touchedSpot) {
                        return LineTooltipItem(
                          '${axis[touchedSpot.barIndex].getY(touchedSpot.y).toStringAsFixed(1)}${records[axis[touchedSpot.barIndex].label]!.unit}', const TextStyle(),
                        );
                      }).toList();
                    },
                  ),
                ),
              ), 
            ),
          ),
        ),
      ],
    );
  }
}

