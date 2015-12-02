#include <tulip/BoundingBox.h>
#include <tulip/Coord.h>
#include <tulip/BooleanProperty.h>

#include "ZoomAndPanAnimation.h"
#include "GlRect2D.h"
#include "GlScene.h"
#include "GlLayer.h"
#include "TimeMeasurer.h"

#include "RectangleZoomInteractor.h"

static ZoomAndPanAnimation *zoomAndPanAnimation = NULL;
static const unsigned int baseAnimDuration = 1000;
static bool animating = false;
static double animDuration = 0;
static TimeMeasurer tm;
static const unsigned int delay = 40;

static void animate(void *value) {
  AnimateParams *animParams = reinterpret_cast<AnimateParams*>(value);
  double t = tm.getElapsedTime() / animDuration;
  if (t < 1.0) {
    zoomAndPanAnimation->zoomAndPanAnimationStep(t);
    animParams->scene->setBackupBackBuffer(false);
    animParams->scene->requestDraw();
    timerFunc(delay, animate, animParams);
  } else {
    zoomAndPanAnimation->zoomAndPanAnimationStep(1.0);
    animParams->scene->setBackupBackBuffer(true);
    animParams->scene->requestDraw();
    delete zoomAndPanAnimation;
    zoomAndPanAnimation = NULL;
    animating = false;
  }
}

RectangleZoomInteractor::RectangleZoomInteractor(GlScene *glScene) :
  _firstX(-1), _firstY(-1), _curX(-1), _curY(-1), _dragStarted(false) {
  setScene(glScene);
  _animParams.scene = glScene;
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

      zoomAndPanAnimation = new ZoomAndPanAnimation(camera, bb, baseAnimDuration);
      if (zoomAndPanAnimation->canDoZoomAndPan()) {
        animDuration = zoomAndPanAnimation->getAnimationDuration();
        animating = true;
        _animParams.scene = _glScene;
        tm.reset();
        timerFunc(delay, animate, &_animParams);
      } else {
        delete zoomAndPanAnimation;
        zoomAndPanAnimation = NULL;
      }

      _firstX = _curX = -1;
      _firstY = _curY = -1;
      _glScene->requestDraw();
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
    _glScene->requestDraw();
    return true;
  }
  return false;
}

bool RectangleZoomInteractor::keyboardCallback(const std::string &keyStr, const int & /*modifiers*/) {
  if (!_glScene || animating) return false;

  if (keyStr == "c") {
    Camera *camera = _glScene->getMainLayer()->getCamera();
    zoomAndPanAnimation = new ZoomAndPanAnimation(camera, camera->getSceneBoundingBox(), baseAnimDuration);
    animDuration = zoomAndPanAnimation->getAnimationDuration();
    animating = true;
    _animParams.scene = _glScene;
    tm.reset();
    timerFunc(delay, animate, &_animParams);
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

