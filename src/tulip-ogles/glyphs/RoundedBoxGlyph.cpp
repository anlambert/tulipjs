#include "RoundedBoxGlyph.h"

#include "../Utils.h"

#include <algorithm>

using namespace std;
using namespace tlp;

static vector<Coord> createRoundedRect(const Size &size) {
  float radius = min(size[0] / 4, size[1] / 4);
  float radiusL = radius / size[0];
  float radiusH = radius / size[1];

  float wi = 1.0 - 2*radiusL;
  float hi = 1.0 - 2*radiusH;

  Coord P1 = Coord(-0.5, 0.5) + Coord(radiusL, -radiusH);
  Coord P2 = P1 + Coord(wi, 0);
  Coord P3 = P2 + Coord(0, -hi);
  Coord P4 = P1 + Coord(0, -hi);

  int steps = 20;
  float delta = (M_PI/2) / steps;

  vector<Coord> boxPoints;
  boxPoints.resize(steps * 4);

  for (int i = 0 ; i < steps; ++i ) {
    float w = delta + i * delta;
    float x = -cos(w);
    float y = sin(w);
    Coord p = P1 + Coord(x, y) * Coord(radiusL, radiusH);
    boxPoints[i] = p;

    w = delta + (steps - i - 1) * delta;
    x = cos(w);
    y = sin(w);
    p = P2 + Coord(x, y) * Coord(radiusL, radiusH);
    boxPoints[steps + i] = p;

    w = delta +  i * delta;
    x = cos(w);
    y = -sin(w);
    p = P3 + Coord(x, y) * Coord(radiusL, radiusH);
    boxPoints[2 * steps + i] = p;

    w = delta + (steps - i - 1) * delta;
    x = -cos(w);
    y = -sin(w);
    p = P4 + Coord(x, y) * Coord(radiusL, radiusH);
    boxPoints[3 * steps + i] = p;
  }

  return boxPoints;
}

static Coord computeCircleArcMidPoint(const Coord &start, const Coord &end, const Coord &center) {
  float radius = start.dist(center);
  float c=atan2(start[1]+end[1], start[0]+end[0]);
  return Coord(center.x() + radius*cos(c), center.y() + radius*sin(c));
}

static Coord minIncludeBBSquare = computeCircleArcMidPoint(Coord(-0.25, -0.5), Coord(-0.5, -0.25), Coord(-0.25, -0.25));
static Coord maxIncludeBBSquare = -minIncludeBBSquare;

RoundedBoxGlyph::RoundedBoxGlyph() {
  _vertices.push_back(Coord());
  vector<Coord> contour = createRoundedRect(Size(1,1,1));
  _vertices.insert(_vertices.end(), contour.begin(), contour.end());
  for (unsigned int i = 0 ; i < contour.size() - 1 ; ++i) {
    _indices.push_back(0);
    _indices.push_back(i+1);
    _indices.push_back(i+2);
  }
  _indices.push_back(0);
  _indices.push_back(contour.size());
  _indices.push_back(1);
  for (size_t i = 0 ; i < _vertices.size() - 2 ; ++i) {
    _outlineIndices.push_back(i+1);
    _outlineIndices.push_back(i+2);
  }
  _outlineIndices.push_back(_vertices.size() - 1);
  _outlineIndices.push_back(1);
}

void RoundedBoxGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
  boundingBox[0] = minIncludeBBSquare;
  boundingBox[1] = maxIncludeBBSquare;
}

Coord RoundedBoxGlyph::getAnchor(const Coord &vector) const {
  Coord v(vector);
  float x, y, z, fmax;
  v.get(x, y, z);
  v.setZ(0.0f);
  fmax = std::max(fabsf(x), fabsf(y));

  if (fmax > 0.0f)
    return v * (0.5f / fmax);
  else
    return v;
}

