#ifndef ZoomAndPanAnimation_H_
#define ZoomAndPanAnimation_H_

#include <tulip/Vector.h>
#include <tulip/Coord.h>
#include <tulip/BoundingBox.h>

#include "Camera.h"

class ZoomAndPanAnimation  {

public :

  ZoomAndPanAnimation(Camera *camera, const tlp::BoundingBox &boundingBox, const unsigned int animationDuration = 1000, const bool optimalPath = true, const float velocity = 1.1f, const double p = sqrt(1.6));

  virtual ~ZoomAndPanAnimation() {}

  float getAnimationDuration() const {
    return _animationDuration;
  }

  void zoomAndPanAnimationStep(double t);

  bool canDoZoomAndPan() const {
    return _doZoomAndPan;
  }

protected :

  Camera *_camera;
  tlp::Vec4i _viewport;
  unsigned int _animationDuration;
  bool _optimalPath;
  double _p;
  tlp::Coord _camCenterStart, _camCenterEnd;
  double _w0, _w1, _u0, _u1, _b0, _b1, _r0, _r1, _S, _sA, _sB, _wm;
  float _zoomAreaWidth, _zoomAreaHeight;
  bool _doZoomAndPan;

};

void adjustViewToBoundingBox(Camera *camera, const tlp::BoundingBox &boundingBox);

#endif /* ZoomAndPanAnimation_H_ */
