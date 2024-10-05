import 'dart:math';

import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'recording.dart';

class AltitudeChart extends StatefulWidget {
  final Recording? recording;
  const AltitudeChart({super.key, this.recording});

  @override
  State<AltitudeChart> createState() => _AltitudeChartState();
}

class Scale {
  double min = 0;
  double max = 0;
  double tick = 0;
}

class YAxis {
  Recording recording;
  int numLabels;
  String label;
  double scalar = 0;
  Scale scale = Scale();
  int precision = 2;
  Color colour = Colors.blue;
  List<FlSpot> spots = [];

  double getY(double y) => y * scalar + scale.min;

  void getScale(double minY, maxY) {
    double log10(x)=>log(x)/log(10);
    
    double minTickSize = (maxY - minY) / numLabels;
    double tick = 1;
    if (minTickSize != 0) {
      tick = pow(10, log10(minTickSize).floor()) * 1.0;
    }

    var residue = minTickSize / tick;
    if (residue > 5) {
      tick *= 10;
    } else if (residue > 2) {
      tick *= 5;
    } else if (residue > 1) {
      tick *= 2;
    } else {
      tick * 1;
    }

    scale.min = (minY / tick).floor() * tick;
    scale.max = scale.min + (numLabels) * tick;
    scale.tick = tick;
  }
  
  YAxis(this.recording, this.label, this.numLabels) {
    colour = records[label]!.colour;

    // get the min and max values for this axis
    var minY = recording.values[label]!.reduce((a, b)=>(a[1] < b[1] ? a : b))[1]; 
    var maxY = recording.values[label]!.reduce((a, b)=>(a[1] > b[1] ? a : b))[1]; 


    getScale(minY, maxY);

    if (scale.min == scale.max) {
      scalar = 1;
    } else {
      scalar = (scale.max - scale.min);
    }

    spots = [...recording.values[label]!.map((e)=>FlSpot(e[0], (e[1] - scale.min)/scalar))];
  }
}

Widget customYAxisLabel({required String value, required Color colour}) {
  return Text(
    value,
    style: TextStyle(fontSize: 12, color: colour),   
  );
}

Widget customYAxis(YAxis axis) {
  List<double> labels = [];
  for (int i = axis.numLabels; i >= 0; i--) {
    labels.add(axis.scale.min + i * axis.scale.tick);
  }
  return Padding(
    padding:const EdgeInsets.symmetric(vertical: 0, horizontal: 10),
    child: Column(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        for (var label in labels)
        customYAxisLabel(value: label.toStringAsFixed(axis.precision), colour:axis.colour),
      ],
    )
  );
}
 
class _AltitudeChartState extends State<AltitudeChart> {
  Recording? recording; 
  static const numLabels = 20;

  @override
  void initState() {
  super.initState();
  }

  @override
  Widget build(BuildContext context) {
    List<YAxis> axis = [];
    if (widget.recording != null) {
      for (var recordLabel in widget.recording!.values.keys) {
        if (records[recordLabel]!.plot) {
          axis.add(YAxis(widget.recording!, recordLabel, numLabels));
        }
      }
    }

    return Row(
      children:[
        Padding(
          padding: const EdgeInsets.only(bottom:20),
          child:
            Row(
              children:[
                for (var ax in axis) customYAxis(ax)
              ],
            ), 
        ),
        Expanded(
          child: Padding(
            padding: const EdgeInsets.only(left:20, top:4),
              child: LineChart(
                LineChartData (
                //borderData: FlBorderData(border: const Border(bottom: BorderSide(), left: BorderSide())),
                gridData:const FlGridData(horizontalInterval: 1/numLabels),
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
                minY: 0, //(dataLength > 1 ? ((points[0].reduce((cur, next) => cur.y < next.y ? cur: next)).y / 10).floor() * 10.0 : 0),
                maxY: 1.0, //(dataLength > 1 ? ((points[0].reduce((cur, next) => cur.y > next.y ? cur: next)).y / 10).ceil() * 10.0 : 0),
                lineTouchData: LineTouchData(
                  touchTooltipData: LineTouchTooltipData(
                    fitInsideHorizontally: true,
                    fitInsideVertically: true,
                    getTooltipItems: (touchedSpots) {
                      return touchedSpots.map((LineBarSpot touchedSpot) {
                        return LineTooltipItem(
                          '${axis[touchedSpot.barIndex].getY(touchedSpot.y).toStringAsFixed(records[axis[touchedSpot.barIndex].label]!.precision)}${records[axis[touchedSpot.barIndex].label]!.unit}', const TextStyle(),
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

