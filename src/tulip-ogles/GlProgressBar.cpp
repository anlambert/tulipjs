#include "GlProgressBar.h"

#include "NanoVGManager.h"
#include "LabelsRenderer.h"
#include "Camera.h"

#include <sstream>

using namespace std;
using namespace tlp;

static void fillRoundedRect(NVGcontext* vg, float x, float y, float w, float h, float cornerRadius, NVGpaint &bg) {
  nvgBeginPath(vg);
  nvgRoundedRect(vg, x, y, w, h, cornerRadius);
  nvgFillPaint(vg, bg);
  nvgFill(vg);
}

static void strokeRoundedRect(NVGcontext* vg, float x, float y, float w, float h, float cornerRadius, NVGcolor &c, float strokeWidth = 1.0f) {
  nvgBeginPath(vg);
  nvgRoundedRect(vg, x, y, w, h, cornerRadius);
  nvgStrokeWidth(vg, strokeWidth);
  nvgStrokeColor(vg, c);
  nvgStroke(vg);
}

static const float undefinedProgressBarPercentWidth = 0.1f;

GlProgressBar::GlProgressBar(const tlp::Coord &center, float width, float height,
                             const tlp::Color &fillColor, const tlp::Color &outlineColor,
                             const tlp::Color &textColor) :
  _center(center), _width(width), _height(height), _fillColor(fillColor),
  _outlineColor(outlineColor), _textColor(textColor), _percent(0),
  _undefinedProgressCurrentBarOffset(0), _undefinedProgressStep(0.05f) {
  updateBoundingBox();
}

void GlProgressBar::draw(const Camera &camera, const Light &, bool) {
  int percent = _percent;

  if (percent > 100)
    percent = 100;

  std::ostringstream oss;
  oss.str("");
  oss << percent << "%";

  float pbWidth = _width;
  float pbHeight = _height;

  Color textColor = _textColor;

  tlp::Coord bl = _center - tlp::Coord(pbWidth, pbHeight)/2.f;
  tlp::Coord tr = _center + tlp::Coord(pbWidth, pbHeight)/2.f;

  tlp::Vec4i viewport = camera.getViewport();
  NVGcontext* vg = NanoVGManager::instance()->getNanoVGContext();

  nvgBeginFrame(vg, viewport[2], viewport[3], 1.0);

  NVGpaint bg;
  float cornerRadius = _height/2-1;

  float x = _center[0] - _width/2;
  float y = _center[1] - _height/2;
  float w = _width;
  float h = _height;

  NVGcolor fillColor = nvgRGBA(_fillColor[0],_fillColor[1],_fillColor[2],_fillColor[3]);
  NVGcolor outlineColor = nvgRGBA(_outlineColor[0],_outlineColor[1],_outlineColor[2],_outlineColor[3]);

  bg = nvgBoxGradient(vg, x, y, w, h, cornerRadius, 5, nvgRGBA(0,0,0,16), nvgRGBA(0,0,0,92));
  fillRoundedRect(vg, x, y, w, h, cornerRadius, bg);
  strokeRoundedRect(vg, x, y, w, h, cornerRadius, outlineColor);

  if (percent >= 0) {
    float pbWidth2 = (percent/100.f) * pbWidth;
    w = pbWidth2;
    bg = nvgBoxGradient(vg, x, y, w, h, cornerRadius, cornerRadius/2, fillColor, nvgRGBA(0,0,0,10));
    fillRoundedRect(vg, x+1, y+1, w-2, h-2, cornerRadius-1, bg);
  } else {
    _undefinedProgressCurrentBarOffset += _undefinedProgressStep;
    if (_undefinedProgressCurrentBarOffset > 1.0) {
      _undefinedProgressCurrentBarOffset = 1.0 - undefinedProgressBarPercentWidth;
      _undefinedProgressStep = -_undefinedProgressStep;
    }
    if (_undefinedProgressCurrentBarOffset < 0.0) {
      _undefinedProgressCurrentBarOffset = 0.0;
      _undefinedProgressStep = -_undefinedProgressStep;
    }
    float pbWidth2 = undefinedProgressBarPercentWidth * pbWidth;
    tlp::Coord bl2 = bl + tlp::Coord(_undefinedProgressCurrentBarOffset * pbWidth, 0);

    x = bl2[0];
    y = bl2[1];
    w = pbWidth2;

    bg = nvgBoxGradient(vg, x, y, w, h, cornerRadius, cornerRadius/2, fillColor, nvgRGBA(0,0,0,10));
    fillRoundedRect(vg, x+1, y+1, w-2, h-2, cornerRadius-1, bg);
  }

  tlp::BoundingBox textBB;
  textBB[0] = bl + tlp::Coord(0, pbHeight*0.3f);
  textBB[1] = tr - tlp::Coord(0, pbHeight*0.3f);

  if (percent >= 0) {
    LabelsRenderer::instance()->renderOneLabel(camera, oss.str(), textBB, textColor);
  }

  pbWidth = 0.9f * pbWidth;
  float pbHeight2 = 0.9f * pbHeight;
  bl = _center - tlp::Coord(pbWidth, pbHeight2)/2.f;
  tr = _center + tlp::Coord(pbWidth, pbHeight2)/2.f;
  textBB[0] = bl + tlp::Coord(0, pbHeight);
  textBB[1] = tr + tlp::Coord(0, pbHeight);

  LabelsRenderer::instance()->renderOneLabel(camera, _comment, textBB, textColor);

  nvgEndFrame(vg);

}

void GlProgressBar::setPercent(int percent) {
  _percent = percent;
  notifyModified();
}

void GlProgressBar::setComment(const std::string &comment) {
  _comment = comment;
  notifyModified();
}

void GlProgressBar::setCenter(const tlp::Coord &center) {
  _center = center;
  updateBoundingBox();
  notifyModified();
}

void GlProgressBar::setWidth(float width) {
  _width = width;
  updateBoundingBox();
  notifyModified();
}

void GlProgressBar::setHeight(float height) {
  _height = height;
  updateBoundingBox();
  notifyModified();
}

void GlProgressBar::setFillColor(const tlp::Color &fillColor) {
  _fillColor = fillColor;
  notifyModified();
}

void GlProgressBar::setOutlineColor(const tlp::Color &outlineColor) {
  _outlineColor = outlineColor;
  notifyModified();
}

void GlProgressBar::setTextColor(const tlp::Color &textColor) {
  _textColor = textColor;
  notifyModified();
}

void GlProgressBar::updateBoundingBox() {
  _boundingBox[0] = _center - Coord(_width, 2* _height)/2.f;
  _boundingBox[1] = _center + Coord(_width, 2* _height)/2.f;
}
