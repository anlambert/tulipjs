#include "TimeMeasurer.h"

#ifndef __EMSCRIPTEN__
#include <omp.h>
#else
#include <emscripten.h>
#endif

TimeMeasurer::TimeMeasurer() : _startTime(0), _endTime(0), _stopped(true) {}

void TimeMeasurer::start() {
#ifndef __EMSCRIPTEN__
  _startTime = omp_get_wtime();
#else
  _startTime = emscripten_get_now();
#endif
  _stopped = false;
}

void TimeMeasurer::stop() {
#ifndef __EMSCRIPTEN__
  _endTime = omp_get_wtime();
#else
  _endTime = emscripten_get_now();
#endif
  _stopped = true;
}

double TimeMeasurer::getElapsedTimeInSeconds() {
  if (!_stopped) stop();
#ifndef __EMSCRIPTEN__
  return (_endTime - _startTime);
#else
  return (_endTime - _startTime) / 1000;
#endif
}
