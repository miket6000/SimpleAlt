import 'dart:math';
import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'recording.dart';

GlobalKey screenshotKey = GlobalKey();

class AltitudeChart extends StatefulWidget {
  final Recording? recording;
  const AltitudeChart({super.key, this.recording});

  @override
  State<AltitudeChart> createState() => _AltitudeChartState();
}

class Scale {
  double min;
  double max;
  double tick = 1;
  double range = 1;
  final int numTicks;

  update(double min, double max) {
    double log10(x)=>log(x)/log(10);
    
    double minTickSize = (max - min) / numTicks;
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

    this.min = (min / tick).floor() * tick;
    this.max = this.min + (numTicks) * tick;
    range = (this.min == this.max) ? 1 : this.max - this.min;
  }

  Scale({this.min = 0, this.max = 1, required this.numTicks}) {
    update(min, max);
  }
}

extension X<T> on List<T> {
  List<T> everyNth(int n) {
    if (n == 0) n = 1;
    return [for (var i = 0; i < length; i += n) this[i]];
  }
}

class YAxis {
  Recording recording;
  int numTicks;
  String label;
  late Scale scale;
  int precision = 2;
  Color colour = Colors.blue;
  bool get visible => records[label]!.plot;
  List<FlSpot> spots = [];
  List<List<double>> visibleValues = [];

  double realY(double y) => y * scale.range + scale.min;
  double graphY(double y) => (y - scale.min) / scale.range;
  
  update(RangeValues zoom) {
    // get all values within current zoom window
    var length = recording.values[label]!.length;
    visibleValues = recording.values[label]!.sublist((length * zoom.start).toInt(), (length * zoom.end).toInt());
    
    // reduce down to max 2000 points to keep the UI snappy
    int reduction = visibleValues.length ~/ 2000;
    visibleValues = visibleValues.everyNth(reduction);

    // update the scale to fit the new values
    var min = visibleValues.isNotEmpty ? visibleValues.reduce((a, b)=>(a[1] < b[1] ? a : b))[1] : scale.min; 
    var max = visibleValues.isNotEmpty ? visibleValues.reduce((a, b)=>(a[1] > b[1] ? a : b))[1] : scale.max;
    scale.update(min, max);

    // and get the new spots.
    spots = [...visibleValues.map((e)=>FlSpot(e[0], graphY(e[1])))]; 
  }

  YAxis(this.recording, this.label, this.numTicks) {
    colour = records[label]!.colour;
    visibleValues = recording.values[label]!;
    scale = Scale(numTicks:numTicks);
    update(const RangeValues(0, 1.0));
  }
}

Widget customYAxis(YAxis axis) {
  List<double> labels = [];
  for (int i = axis.numTicks; i >= 0; i--) {
    labels.add(axis.scale.min + i * axis.scale.tick);
  }
  return Padding(
    padding:const EdgeInsets.symmetric(vertical: 0, horizontal: 10),
    child: Column(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        for (var label in labels)
          Text(
            label.toStringAsFixed(axis.precision),
            style: TextStyle(fontSize: 12, color: axis.colour),   
          )
      ],
    )
  );
}
 
class _AltitudeChartState extends State<AltitudeChart> {
  static const numTicks = 20;
  RangeValues _zoomSliderValues = const RangeValues(0, 1.0);
  List<YAxis> axis = [];
  Recording? lastRecording;
  
  @override
  Widget build(BuildContext context) {
    if (widget.recording != lastRecording && widget.recording != null) {
      lastRecording = widget.recording;
      axis.clear();
      for (var recordLabel in widget.recording!.values.keys) {
        axis.add(YAxis(widget.recording!, recordLabel, numTicks));
      }
    }
    var visibleAxis = axis.where((e)=>e.visible).toList();

    return RepaintBoundary(
      key: screenshotKey,
      child: Column(
        children:[
          Expanded(child:
          Row(
            children:[
              Padding(
                padding: const EdgeInsets.only(bottom:20),
                child:
                  Row(
                    children:[
                      for (var ax in visibleAxis) customYAxis(ax)
                    ],
                  ), 
              ),
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.only(left:20, top:4),
                    child: LineChart(
                      //key: ValueKey(_zoomSliderValues),
                      LineChartData (
                      //borderData: FlBorderData(border: const Border(bottom: BorderSide(), left: BorderSide())),
                      gridData:const FlGridData(horizontalInterval: 1/numTicks),
                      lineBarsData: [
                        for (var ax in visibleAxis)
                          LineChartBarData(spots: ax.spots, dotData: const FlDotData(show: false,), color:ax.colour),
                      ],
                      titlesData: const FlTitlesData(
                        topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                        rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                        leftTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)), 
                        //bottomTitles: AxisTitles(sideTitles: SideTitles(showTitles: true, interval: 20)),
                      ),
                      minY: 0,
                      maxY: 1.0,
                      lineTouchData: LineTouchData(
                        touchTooltipData: LineTouchTooltipData(
                          fitInsideHorizontally: true,
                          fitInsideVertically: true,
                          getTooltipItems: (touchedSpots) {
                            return touchedSpots.map((LineBarSpot touchedSpot) {
                              return LineTooltipItem(
                                '${visibleAxis[touchedSpot.barIndex].realY(touchedSpot.y).toStringAsFixed(records[visibleAxis[touchedSpot.barIndex].label]!.precision)}${records[visibleAxis[touchedSpot.barIndex].label]!.unit}', 
                                const TextStyle(),
                              );
                            }).toList();
                          },
                        ),
                      ),
                    ), 
                    duration: const Duration(milliseconds: 0), // Animation duration, setting to zero to turn off
                  ),
                ),
              ),
            ],
          ),
          ),
          SizedBox( // the range slider needs it's height defined otherwise the 'Expanded' has no bounds and explodes
            height: 30,
            child:
            RangeSlider(
              min: 0,
              max: 1.0,
              values: _zoomSliderValues, 
              onChanged: (RangeValues values) {
                for (var ax in axis) {
                  ax.update(values);
                }
              setState(() {
                _zoomSliderValues = values;
              });
            
              },
            )
          ),
        ],
      ),
    );
  }
}

