#include "DiamondGlyph.h"

#include "../Utils.h"

using namespace std;
using namespace tlp;

DiamondGlyph::DiamondGlyph() {
  _vertices = computeRegularPolygon(4);
  _indices.push_back(0);
  _indices.push_back(1);
  _indices.push_back(3);
  _indices.push_back(1);
  _indices.push_back(2);
  _indices.push_back(3);
  for (size_t i = 0 ; i < _vertices.size() - 1 ; ++i) {
    _outlineIndices.push_back(i);
    _outlineIndices.push_back(i+1);
  }
  _outlineIndices.push_back(_vertices.size()-1);
  _outlineIndices.push_back(0);
}

void DiamondGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
  boundingBox[0] = Coord(-0.35f, -0.35f, 0);
  boundingBox[1] = Coord(0.35f, 0.35f, 0);
}

Coord DiamondGlyph::getAnchor(const Coord &vector) const {
  Coord v(vector);
  float x, y, z;
  v.get(x, y, z);
  // initialize anchor as top corner
  Coord anchor(0, 0.5, 0);
  float distMin = x*x + ((y - 0.5) * (y - 0.5));
  // check with the right corner
  float dist = ((x - 0.5) * (x - 0.5)) + y*y;

  if (distMin > dist) {
    distMin = dist;
    anchor = Coord(0.5, 0, 0);
  }

  // check with the bottom corner
  dist = x*x + ((y + 0.5)*(y + 0.5));

  if (distMin > dist) {
    distMin = dist;
    anchor = Coord(0, -0.5, 0);
  }

  // check with left corner
  if (distMin > ((x + 0.5) * (x + 0.5)) + y*y)
    return Coord(-0.5, 0, 0);

  return anchor;
}
