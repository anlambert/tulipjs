#include <tulip/Vector.h>

#include "GlScene.h"
#include "GlLayer.h"
#include "GlGraph.h"
#include "ZoomAndPanInteractor.h"

ZoomAndPanInteractor::ZoomAndPanInteractor(GlScene *glScene) : _mouseButton(-1), _lastX(-1), _lastY(-1), _dragStarted(false) {
  setScene(glScene);
}

bool ZoomAndPanInteractor::mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int & /*modifiers*/) {
  if (!_glScene) return false;
  tlp::Vec4i viewport = _glScene->getViewport();
  if (x < viewport[0] || x > viewport[2] || y < viewport[1] || y > viewport[3]) return false;
  _mouseButton = button;
  if (button == LEFT_BUTTON) {
    if (state == DOWN) {
      _lastX = x;
      _lastY = y;
      _dragStarted = true;
      return true;
    } else if (state == UP) {
      _lastX = -1;
      _lastY = -1;
      _dragStarted = false;
      return true;
    }
  } else if (button == WHEEL && state == UP) {
    _glScene->zoomXY(x, y, 1);
    _glScene->requestDraw();
    return true;
  } else if (button == WHEEL && state == DOWN) {
    _glScene->zoomXY(x, y, -1);
    _glScene->requestDraw();
    return true;
  }
  return false;
}

bool ZoomAndPanInteractor::mouseMoveCallback(int x, int y, const int &modifiers) {
  if (!_glScene) return false;
  tlp::Vec4i viewport = _glScene->getViewport();
  if (x < viewport[0] || x > viewport[2] || y < viewport[1] || y > viewport[3]) return false;
  if (_lastX < viewport[0] || _lastX > viewport[2] || _lastY < viewport[1] || _lastY > viewport[3]) return false;
  if (_mouseButton == LEFT_BUTTON && !modifiers && _dragStarted) {
    _glScene->translate(_lastX - x, y - _lastY);
    _lastX = x;
    _lastY = y;
    _glScene->requestDraw();
    return true;
  } else if (_mouseButton == LEFT_BUTTON && modifiers == ACTIVE_CTRL) {
    _glScene->rotate(0, 0, x - _lastX);
    _lastX = x;
    _lastY = y;
    _glScene->requestDraw();
    return true;
  } else if (_mouseButton == LEFT_BUTTON && modifiers == ACTIVE_SHIFT) {

    int deltaX,deltaY;
    deltaX=_lastX-x;
    deltaY=_lastY-y;

    if (abs(deltaX)>abs(deltaY))
      deltaY=0;
    else
      deltaX=0;

    if (deltaY!=0) _glScene->rotate(deltaY,0,0);
    if (deltaX!=0) _glScene->rotate(0,deltaX,0);

    _lastX = x;
    _lastY = y;
    _glScene->requestDraw();
    return true;
  }
  return false;
}

bool ZoomAndPanInteractor::keyboardCallback(const std::string &keyStr, const int & /*modifiers*/) {
  if (!_glScene) return false;
  GlGraph *glGraph = dynamic_cast<GlGraph*>(_glScene->getMainLayer()->findGlEntity("graph"));
  if (keyStr == "c") {
    _glScene->centerScene();
    _glScene->requestDraw();
    return true;
  } else if (keyStr == "F1") {
    if (glGraph) {
      bool edgeColorsInterpolation = glGraph->getRenderingParameters().interpolateEdgesColors();
      glGraph->getRenderingParameters().setInterpolateEdgesColors(!edgeColorsInterpolation);
      _glScene->requestDraw();
      return true;
    }
  } else if (keyStr == "F2") {
    if (glGraph) {
      bool edgeSizesInterpolation = glGraph->getRenderingParameters().interpolateEdgesSizes();
      glGraph->getRenderingParameters().setInterpolateEdgesSizes(!edgeSizesInterpolation);
      _glScene->requestDraw();
      return true;
    }
  } else if (keyStr == "F3") {
    if (glGraph) {
      bool edgeExtremities = glGraph->getRenderingParameters().displayEdgesExtremities();
      glGraph->getRenderingParameters().setDisplayEdgesExtremities(!edgeExtremities);
      _glScene->requestDraw();
      return true;
    }
  } else if (keyStr == "F4") {
    if (glGraph) {
      bool edge3d = glGraph->getRenderingParameters().edges3D();
      glGraph->getRenderingParameters().setEdges3D(!edge3d);
      glGraph->getRenderingParameters().setBillboardedNodes(!edge3d);
      glGraph->getRenderingParameters().setBillboardedLabels(!edge3d);
      _glScene->requestDraw();
      return true;
    }
  } else if (keyStr == "F5") {
    if (glGraph) {
      bool labelsScaled = glGraph->getRenderingParameters().labelsScaled();
      glGraph->getRenderingParameters().setLabelsScaled(!labelsScaled);
      _glScene->requestDraw();
      return true;
    }
  } else if (keyStr == "F6") {
    if (glGraph) {
      bool labels = glGraph->getRenderingParameters().displayNodesLabels();
      glGraph->getRenderingParameters().setDisplayNodesLabels(!labels);
      _glScene->requestDraw();
      return true;
    }
  } else if (keyStr == "F7") {
    if (glGraph) {
      bool edges = !glGraph->getRenderingParameters().displayEdges();
      glGraph->getRenderingParameters().setDisplayEdges(edges);
      _glScene->requestDraw();
      return true;
    }
  }
  return false;
}
