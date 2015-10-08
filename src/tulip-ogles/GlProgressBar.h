#ifndef GLPROGRESSBAR_H
#define GLPROGRESSBAR_H

#include <tulip/Coord.h>
#include <tulip/Color.h>

#include "GlEntity.h"

class GlProgressBar : public GlEntity {

public:

  GlProgressBar(const tlp::Coord &center, float width, float height,
                const tlp::Color &fillColor, const tlp::Color &outlineColor,
                const tlp::Color &textColor);

  void draw(const Camera &camera, const Light &light, bool pickingMode=false);

  void setPercent(int percent);

  int getPercent() const {
    return _percent;
  }

  void setComment(const std::string &comment);

  std::string getComment() const {
    return _comment;
  }

  void setCenter(const tlp::Coord &center);

  tlp::Coord getCenter() const {
    return _center;
  }

  void setWidth(float width);

  float getWidth() const {
    return _width;
  }

  void setHeight(float height);

  float getHeight() const {
    return _height;
  }

  void setFillColor(const tlp::Color &fillColor);

  tlp::Color getFillColor() const {
    return _fillColor;
  }

  void setOutlineColor(const tlp::Color &outlineColor);

  tlp::Color getOutlineColor() const {
    return _outlineColor;
  }

  void setTextColor(const tlp::Color &textColor);

  tlp::Color getTextColor() const {
    return _textColor;
  }

private:

  void updateBoundingBox();

  tlp::Coord _center;
  float _width;
  float _height;
  tlp::Color _fillColor;
  tlp::Color _outlineColor;
  tlp::Color _textColor;
  int _percent;
  std::string _comment;
  float _undefinedProgressCurrentBarOffset;
  float _undefinedProgressStep;

};

#endif // GLPROGRESSBAR_H
