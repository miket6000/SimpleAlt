import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';

class AltitudeChart extends StatefulWidget {
  final List<FlSpot> points;
  const AltitudeChart({super.key, required this.points});

  @override
  State<AltitudeChart> createState() => _AltitudeChartState();
}

class _AltitudeChartState extends State<AltitudeChart> {
  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return LineChart(
      LineChartData(
        //borderData: FlBorderData(border: const Border(bottom: BorderSide(), left: BorderSide())), 
        lineBarsData: [
          LineChartBarData(spots: widget.points, dotData: const FlDotData(show: false,),)
        ],
        titlesData: const FlTitlesData(
          topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
          rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
//          bottomTitles: AxisTitles(sideTitles: SideTitles(showTitles: true, interval: 20)),
        ),
        maxX: (widget.points.length > 1 ? ((widget.points.reduce((cur, next) => cur.x > next.x ? cur: next)).x / 10).ceil() * 10.0 : 0),
        minY: (widget.points.length > 1 ? ((widget.points.reduce((cur, next) => cur.y < next.y ? cur: next)).y / 10).floor() * 10.0 : 0),
        maxY: (widget.points.length > 1 ? ((widget.points.reduce((cur, next) => cur.y > next.y ? cur: next)).y / 10).ceil() * 10.0 : 0),
        lineTouchData: LineTouchData(
          touchTooltipData: LineTouchTooltipData(
            fitInsideHorizontally: true,
            fitInsideVertically: true,
            getTooltipItems: (touchedSpots) {
              return touchedSpots.map((LineBarSpot touchedSpot) {
                return LineTooltipItem(
                  '${widget.points[touchedSpot.spotIndex].y.toStringAsFixed(1)}m', const TextStyle(),
                );
              }).toList();
            },
          )
        )  
      ),
    );
  }
}

