#include "GlProgressBar.h"
#include "GlRect2D.h"
#include "LabelsRenderer.h"

using namespace std;
using namespace tlp;

static const float undefinedProgressBarPercentWidth = 0.1f;

GlProgressBar::GlProgressBar(const tlp::Coord &center, float width, float height,
                             const tlp::Color &fillColor, const tlp::Color &outlineColor,
                             const tlp::Color &textColor) :
  _center(center), _width(width), _height(height), _fillColor(fillColor),
  _outlineColor(outlineColor), _textColor(textColor), _percent(0),
  _undefinedProgressCurrentBarOffset(0), _undefinedProgressStep(0.05f) {
  updateBoundingBox();
}

void GlProgressBar::draw(const Camera &camera, const Light &light, bool pickingMode) {
  int percent = _percent;

  if (percent > 100)
    percent = 100;

  std::ostringstream oss;
  oss.str("");
  oss << percent << "%";

  float pbWidth = _width;
  float pbHeight = _height;

  Color fillColor = _fillColor;
  Color outlineColor = _outlineColor;
  Color textColor = _textColor;

  if (pickingMode) {
    fillColor = outlineColor = textColor = _pickingColor;
  }

  tlp::Coord bl = _center - tlp::Coord(pbWidth, pbHeight)/2.f;
  tlp::Coord tr = _center + tlp::Coord(pbWidth, pbHeight)/2.f;

  GlRect2D rect(bl, tr, 0, fillColor, outlineColor);
  rect.setFilled(false);
  rect.draw(camera, light);

  if (percent >= 0) {
    float pbWidth2 = (percent/100.f) * pbWidth;
    tlp::Coord tr2 = bl + tlp::Coord(pbWidth2, pbHeight);
    GlRect2D rect2(bl, tr2, 0, fillColor);
    rect2.draw(camera, light);
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
    tlp::Coord tr2 = bl2 + tlp::Coord(pbWidth2, pbHeight);
    GlRect2D rect2(bl2, tr2, 0, fillColor);
    rect2.draw(camera, light);
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
