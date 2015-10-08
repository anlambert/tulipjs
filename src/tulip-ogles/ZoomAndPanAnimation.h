/*
 *
 * This file is part of Tulip (www.tulip-software.org)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux 1 and Inria Bordeaux - Sud Ouest
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */
#ifndef ZoomAndPanAnimation_H_
#define ZoomAndPanAnimation_H_

#include <tulip/Vector.h>
#include <tulip/Coord.h>
#include <tulip/BoundingBox.h>

#include "Camera.h"

class ZoomAndPanAnimation  {

public :

  ZoomAndPanAnimation(Camera *camera, const tlp::BoundingBox &boundingBox, const int nbAnimationSteps = 50, const float velocity = 1.1, const bool optimalPath = true, const double p = sqrt(1.6));

  virtual ~ZoomAndPanAnimation() {}

  inline int getNbAnimationsStep() const {
    return nbAnimationSteps;
  }

  inline void setNbAnimationSteps(const int nbAnimationSteps) {
    this->nbAnimationSteps = nbAnimationSteps;
  }

  void zoomAndPanAnimationStep(int animationStep);

  float getDurationFactor() const {
      return durationFactor;
  }

  bool canDoZoomAndPan() const {
      return doZoomAndPan;
  }

protected :

  Camera *camera;
  tlp::Vec4i viewport;
  int nbAnimationSteps;
  bool optimalPath;
  double p;
  tlp::Coord camCenterStart, camCenterEnd;
  double w0, w1, u0, u1, b0, b1, r0, r1, S, sA, sB, wm;
  float zoomAreaWidth, zoomAreaHeight;
  bool doZoomAndPan;
  float durationFactor;

};

#endif /* ZoomAndPanAnimation_H_ */
