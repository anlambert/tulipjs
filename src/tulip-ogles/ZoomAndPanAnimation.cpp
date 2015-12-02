#include "ZoomAndPanAnimation.h"

#include <algorithm>

using namespace std;
using namespace tlp;

ZoomAndPanAnimation::ZoomAndPanAnimation(Camera *camera, const BoundingBox &boundingBox, const unsigned int animationDuration, const bool optimalPath, const float velocity, const double p) :
  _camera(camera),_viewport(camera->getViewport()), _animationDuration(animationDuration), _optimalPath(optimalPath), _p(p),
  _camCenterStart(camera->getCenter()),_camCenterEnd(Coord(boundingBox.center())) {

  if (boundingBox.width() == 0 || boundingBox.height() == 0) {
    _doZoomAndPan = false;
    return;
  }

  _camCenterEnd[2] = _camCenterStart[2];

  Coord blScene(camera->screenTo3DWorld(Coord(0, 0, 0)));
  Coord trScene(camera->screenTo3DWorld(Coord(_viewport[2], _viewport[3], 0)));

  BoundingBox sceneBB;
  sceneBB.expand(blScene);
  sceneBB.expand(trScene);

  _zoomAreaWidth = boundingBox[1][0] - boundingBox[0][0];
  _zoomAreaHeight = boundingBox[1][1] - boundingBox[0][1];

  float aspectRatio = _viewport[2] / static_cast<float>(_viewport[3]);

  if (_zoomAreaWidth > (aspectRatio * _zoomAreaHeight)) {
    _w0 = sceneBB[1][0] - sceneBB[0][0];
    _w1 = _zoomAreaWidth;
  }
  else {
    _w0 = sceneBB[1][1] - sceneBB[0][1];
    _w1 = _zoomAreaHeight;
  }

  _u0 = 0;
  _u1 = _camCenterStart.dist(_camCenterEnd);

  if (_u1 < 1e-5) _u1 = 0;

  if (optimalPath) {
    if (_u0 != _u1) {
      _b0 = (_w1 * _w1 - _w0 * _w0 + p * p * p * p * _u1 * _u1) / (2 * _w0 * p * p * _u1);
      _b1 = (_w1 * _w1 - _w0 * _w0 - p * p * p * p * _u1 * _u1) / (2 * _w1 * p * p * _u1);
      _r0 = log(-_b0 + sqrt(_b0 * _b0 + 1));
      _r1 = log(-_b1 + sqrt(_b1 * _b1 + 1));

      _S = (_r1 - _r0) / p;
    }
    else {
      _S = abs(log(_w1 / _w0)) / p;
    }
  }
  else {
    _wm = max(_w0, max(_w1, p * p * (_u1 - _u0) / 2));
    _sA = log(_wm / _w0) / p;
    _sB = _sA + p * (_u1 - _u0) / _wm;
    _S = _sB + log(_wm / _w1) / p;
  }

  if (abs(_w0 - _w1) > 1e-3 || _u1 > 0) {
    _doZoomAndPan = true;
  }
  else {
    _doZoomAndPan = false;
  }

  _animationDuration *= _S/velocity;
}

void ZoomAndPanAnimation::zoomAndPanAnimationStep(double t) {
  if (_doZoomAndPan) {
    double s = t * _S;
    double u = 0, w = 0, f = 0;

    if (_optimalPath) {
      if (_u0 != _u1) {
        u = _w0 / (_p * _p) * cosh(_r0) * tanh(_p * s + _r0) - _w0 / (_p * _p) * sinh(_r0) + _u0;
        w = _w0 * cosh(_r0) / cosh(_p * s + _r0);
        f = u / _u1;
      }
      else {
        double k = (_w1 < _w0) ? -1 : 1;
        w = _w0 * exp(k * _p * s);
        f = 0;
      }
    }
    else {
      if (s >= 0 && s < _sA) {
        u = _u0;
        w = _w0 * exp(_p * s);
      }
      else if (s >= _sA && s < _sB) {
        u = _wm * (s - _sA) / _p + _u0;
        w = _wm;
      }
      else {
        u = _u1;
        w = _wm * exp(_p * (_sB - s));
      }

      if (_u0 != _u1) {
        f = u / _u1;
      }
      else {
        f = 0;
      }
    }

    _camera->setCenter(_camCenterStart + (_camCenterEnd - _camCenterStart) * static_cast<float>(f));
    _camera->setEyes(Coord(0, 0, _camera->getSceneRadius()));
    _camera->setEyes(_camera->getEyes() + _camera->getCenter());
    _camera->setUp(Coord(0, 1., 0));

    Coord bbScreenFirst = _camera->worldTo2DScreen(_camera->getCenter() - Coord(w/2, w/2, 0));
    Coord bbScreenSecond = _camera->worldTo2DScreen(_camera->getCenter() + Coord(w/2, w/2, 0));
    float bbWidthScreen = abs(bbScreenSecond.getX() - bbScreenFirst.getX());
    float bbHeightScreen = abs(bbScreenSecond.getY() - bbScreenFirst.getY());
    double newZoomFactor = 0.0;

    float aspectRatio = _viewport[2] / static_cast<float>(_viewport[3]);

    if (_zoomAreaWidth > (_zoomAreaHeight * aspectRatio)) {
      newZoomFactor = _viewport[2] / bbWidthScreen;
    }
    else {
      newZoomFactor = _viewport[3] / bbHeightScreen;
    }

    _camera->setZoomFactor(_camera->getZoomFactor() * newZoomFactor);
  }
}

