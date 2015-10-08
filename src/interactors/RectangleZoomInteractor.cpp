#include <tulip/BoundingBox.h>
#include <tulip/Coord.h>
#include <tulip/BooleanProperty.h>

#include "ZoomAndPanAnimation.h"
#include "GlRect2D.h"
#include "GlScene.h"
#include "GlLayer.h"

#include "RectangleZoomInteractor.h"

static ZoomAndPanAnimation *zoomAndPanAnimation = NULL;
static const float baseAnimDuration = 500;
static const int nbAnimStep = 50;
static float animDuration = 1000;
static bool animating = false;

static void animate(int value) {
  zoomAndPanAnimation->zoomAndPanAnimationStep(value);
  glDraw();
  if (value < nbAnimStep) {
    timerFunc(animDuration/nbAnimStep, animate, value+1);
  } else {
    delete zoomAndPanAnimation;
    zoomAndPanAnimation = NULL;
    animating = false;
  }
}

RectangleZoomInteractor::RectangleZoomInteractor(GlScene *glScene) :
  _firstX(-1), _firstY(-1), _curX(-1), _curY(-1), _dragStarted(false) {
  setScene(glScene);
}

bool RectangleZoomInteractor::mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int & /*modifiers*/) {
  if (!_glScene) return false;
  Camera *camera = _glScene->getMainLayer()->getCamera();
  tlp::Vec4i viewport = camera->getViewport();
  if (x < viewport[0] || x > viewport[2] || y < viewport[1] || y > viewport[3] || animating) return false;
  _mouseButton = button;
  if (button == LEFT_BUTTON) {
    if (state == DOWN) {
      _firstX = _curX = x;
      _firstY = _curY = y;
      _dragStarted = true;
      return true;
    } else if (state == UP && _dragStarted) {
      _dragStarted = false;
      tlp::Coord bbMin = camera->screenTo3DWorld(tlp::Coord(viewport[2] - _firstX, _firstY));
      tlp::Coord bbMax = camera->screenTo3DWorld(tlp::Coord(viewport[2] - _curX, _curY));
      tlp::BoundingBox bb;
      bb.expand(bbMin);
      bb.expand(bbMax);

      zoomAndPanAnimation = new ZoomAndPanAnimation(camera, bb, nbAnimStep);
      if (zoomAndPanAnimation->canDoZoomAndPan()) {
        animDuration = baseAnimDuration * zoomAndPanAnimation->getDurationFactor();
        animating = true;
        timerFunc(animDuration/nbAnimStep, animate, 0);
      } else {
        delete zoomAndPanAnimation;
        zoomAndPanAnimation = NULL;
      }

      _firstX = _curX = -1;
      _firstY = _curY = -1;
      glDraw();
      return true;
    }
  }
  return false;
}

bool RectangleZoomInteractor::mouseMoveCallback(int x, int y, const int & /*modifiers*/) {
  if (!_glScene) return false;
  tlp::Vec4i viewport = _glScene->getMainLayer()->getCamera()->getViewport();
  if (x < viewport[0] || x > viewport[2] || y < viewport[1] || y > viewport[3] || animating) return false;
  if (_mouseButton == LEFT_BUTTON && _dragStarted) {
    _curX = x;
    _curY = y;
    glDraw();
    return true;
  }
  return false;
}

bool RectangleZoomInteractor::keyboardCallback(const std::string &keyStr, const int & /*modifiers*/) {
  if (!_glScene || animating) return false;

  if (keyStr == "c") {
    Camera *camera = _glScene->getMainLayer()->getCamera();
    zoomAndPanAnimation = new ZoomAndPanAnimation(camera, camera->getSceneBoundingBox(), nbAnimStep);
    animDuration = baseAnimDuration * zoomAndPanAnimation->getDurationFactor();
    animating = true;
    timerFunc(animDuration/nbAnimStep, animate, 0);
    return true;
  }
  return false;
}

void RectangleZoomInteractor::draw() {
  if (!_glScene || _firstX != -1) {
    Camera *camera = _glScene->getMainLayer()->getCamera();
    Camera camera2d(false);
    tlp::Vec4i viewport = camera->getViewport();
    camera2d.setViewport(viewport);
    camera2d.initGl();
    tlp::Vec2f bl(std::min(_firstX, _curX), std::min(viewport[3] - _firstY, viewport[3] - _curY));
    tlp::Vec2f tr(std::max(_firstX, _curX), std::max(viewport[3] - _firstY, viewport[3] - _curY));
    GlRect2D rect(bl, tr, 0, tlp::Color(0,0,255,100), tlp::Color::Black);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    rect.draw(camera2d);
    glDisable(GL_BLEND);
  }
}

