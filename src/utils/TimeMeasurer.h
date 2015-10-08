#ifndef TIMEMEASURER_H
#define TIMEMEASURER_H

class TimeMeasurer {

public:

  TimeMeasurer();

  void start();

  void stop();

  double getElapsedTimeInSeconds();


private:

  bool _stopped;
  double _startTime, _endTime;

};

#endif // TIMEMEASURER_H
