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
  double minY = 0;
  double maxY = 0;
  double offset = 0;
  double scalar = 0;
  static int colourIndex = 0;
  static const List<Color> colourList = [Colors.blue, Colors.deepOrange, Colors.green, Colors.pink, Colors.yellow];
  Color colour = colourList[0];
  int precision = 2;
  double getY(double y) => (y - offset) / scalar;

  YAxis(this.minY, this.maxY) {
    if (minY == maxY) {
      scalar = 1;
    } else {
      scalar = (maxY - minY);
    }

    offset = minY;
    colour = colourList[colourIndex++ % colourList.length];
  }
}

// PointData allows multiple columns of data to contain a reference to an axis
class PointData {
  late YAxis axis;
  List<List<double>> data;
  List<FlSpot> spots = []; // scaled data (0..1) for plotting

  PointData(this.data) {
    double minY = data.reduce((current, next)=>(current[1] < next[1] ? current : next))[1];
    double maxY = data.reduce((current, next)=>(current[1] > next[1] ? current : next))[1];
    axis = YAxis(minY, maxY);
    spots = data.map((sample) => FlSpot(sample[0], axis.getY(sample[1]))).toList(); //update graph data
  }
}

class _AltitudeChartState extends State<AltitudeChart> {
    
  List<PointData> pointList = [];

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
    pointList.clear();
    List<List<FlSpot>> points = [];
    if (widget.recording != null) {
      for (var record in records.keys) {
        List<List<double>> points = [];
        for (var row in widget.recording!.rows) {
          points.add([row[0], row[records[record]!.column]]);
        }
        pointList.add(PointData(points));
      }
    }

    return Row(
      children:[
        for (var pd in pointList) customYAxis(pd.axis),
        Expanded(
          child: LineChart(
            LineChartData(
              //borderData: FlBorderData(border: const Border(bottom: BorderSide(), left: BorderSide())), 
              lineBarsData: [
                for (PointData pointData in pointList) 
                  LineChartBarData(spots: pointData.spots, dotData: const FlDotData(show: false,), color:pointData.axis.colour),
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
                        '${points[0][touchedSpot.spotIndex].y.toStringAsFixed(1)}m', const TextStyle(),
                      );
                    }).toList();
                  },
                )
              )  
            ),
          ),
        ),
      ],
    );
  }
}

