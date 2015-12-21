#include "GlRect2D.h"

using namespace std;
using namespace tlp;

GlRect2D::GlRect2D(const Vec2f &bottomLeftCorner, const Vec2f &topRightCorner, const float z, const Color &fillColor) {
  createContour(bottomLeftCorner, topRightCorner, z);
  setFilled(true);
  setFillColor(fillColor);
  setOutlined(false);
}

GlRect2D::GlRect2D(const Vec2f &bottomLeftCorner, const Vec2f &topRightCorner, const float z, const Color &fillColor, const Color &outlineColor) {
  createContour(bottomLeftCorner, topRightCorner, z);
  setFilled(true);
  setFillColor(fillColor);
  setOutlined(true);
  setOutlineColor(outlineColor);
}

void GlRect2D::createContour(const Vec2f &bottomLeftCorner, const Vec2f &topRightCorner, const float z) {
  vector<Coord> contour;
  contour.push_back(Coord(bottomLeftCorner, z));
  contour.push_back(Coord(topRightCorner[0], bottomLeftCorner[1], z));
  contour.push_back(Coord(topRightCorner, z));
  contour.push_back(Coord(bottomLeftCorner[0], topRightCorner[1], z));
  setPolygonContour(contour);
}
