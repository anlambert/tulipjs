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

#ifndef TLPCAMERA_H
#define TLPCAMERA_H

#include <tulip/Coord.h>
#include <tulip/Matrix.h>
#include <tulip/BoundingBox.h>
#include <tulip/Observable.h>

#include <stack>

namespace tlp {
typedef Matrix<float, 4> MatrixGL;
}

class Camera : public tlp::Observable {
public:

  Camera(tlp::Coord _center=tlp::Coord(0,0,0),
         tlp::Coord _eyes=tlp::Coord(0,0,10), tlp::Coord _up=tlp::Coord(0,1,0),
         double _zoomFactor=1.0, double _sceneRadius=10);

  Camera(bool is3d);

  ~Camera();

  void setViewport(const tlp::Vec4i &viewport);
  
  tlp::Vec4i getViewport() const;
  
  void setSceneRadius(double sceneRadius,const tlp::BoundingBox sceneBoundingBox=tlp::BoundingBox());

  tlp::BoundingBox getSceneBoundingBox() const {
    return _sceneBoundingBox;
  }

  void setViewOrtho(bool viewOrtho);

  bool isViewOrtho() const {
    return _viewOrtho;
  }

  tlp::BoundingBox getBoundingBox();

  void set3d(bool is3d) {
    _3d = is3d;
  }

  bool is3d() const {
    return _3d;
  }

  double getSceneRadius() const {
    return _sceneRadius;
  }

  void setZoomFactor(double zoomFactor);
  
  double getZoomFactor() const {
    return _zoomFactor;
  }

  void setEyes(const tlp::Coord& eyes);
  
  tlp::Coord getEyes() const {
    return _eyes;
  }

  void setCenter(const tlp::Coord& center);
  
  tlp::Coord getCenter() const {
    return _center;
  }

  void setUp(const tlp::Coord& up);
  
  tlp::Coord getUp() const {
    return _up;
  }

  void initGl();

  const tlp::MatrixGL &modelviewMatrix() const {
    return _modelviewMatrix;
  }

  void setModelViewMatrix(const tlp::MatrixGL &mdvMat);

  tlp::MatrixGL rotationMatrix() const;

  const tlp::MatrixGL &projectionMatrix() const {
    return _projectionMatrix;
  }

  const tlp::MatrixGL &transformMatrix() const {
    return _transformMatrix;
  }

  const tlp::MatrixGL &transformMatrixBillboard() const {
    return _transformMatrixBillboard;
  }

  const tlp::MatrixGL &normalMatrix() const {
    return _normalMatrix;
  }

  tlp::Coord screenTo3DWorld(const tlp::Coord &point) ;

  tlp::Coord worldTo2DScreen(const tlp::Coord &obj) ;

  void pushProjectionMatrix();
  void popProjectionMatrix();

  void pushModelViewMatrix();
  void popModelViewMatrix();

  void centerScene(tlp::BoundingBox sceneBoundingBox = tlp::BoundingBox());
  
  void translate(const tlp::Vec3f &move);
  void translate(float x, float y, float z);

  void rotateX(float angle);
  void rotateY(float angle);
  void rotateZ(float angle);

  bool hasRotation() const;

private:

  void initProjection(const tlp::Vec4i& viewport);
  void initProjection();
  void initModelView();

  void ortho(float  left,  float  right,  float  bottom,  float  top,  float  nearVal,  float  farVal);
  void frustum(float  left,  float  right,  float  bottom,  float  top,  float  nearVal,  float  farVal);
  void lookAt(float eyeX,  float eyeY,  float eyeZ,  float centerX,  float centerY,  float centerZ,  float upX,  float upY,  float upZ);

  void rotate(float angle, float x, float y, float z);

  void notifyModified();

  bool _mdvMatCoherent;
  bool _projectionMatCoherent;

  tlp::Coord _center, _eyes, _up;
  double _zoomFactor;
  double _sceneRadius;
  tlp::BoundingBox _sceneBoundingBox;
  tlp::Vec4i _viewport;

  tlp::MatrixGL _modelviewMatrix;
  tlp::MatrixGL _projectionMatrix;
  tlp::MatrixGL _transformMatrix;
  tlp::MatrixGL _transformMatrixBillboard;
  tlp::MatrixGL _normalMatrix;

  bool _3d;
  bool _viewOrtho;

  std::stack<tlp::MatrixGL> _projectionMatrixStack;
  std::stack<tlp::MatrixGL> _modelViewMatrixStack;
  std::stack<tlp::Coord> _centerStack;
  std::stack<tlp::Coord> _eyesStack;
  std::stack<tlp::Coord> _upStack;


};

#endif
