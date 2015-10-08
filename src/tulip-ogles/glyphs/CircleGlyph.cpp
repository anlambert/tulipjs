#include "CircleGlyph.h"

#include "../Utils.h"

using namespace std;
using namespace tlp;

static const unsigned int nbContourPoints = 30;

CircleGlyph::CircleGlyph() {
  _vertices.push_back(Coord());
  vector<Coord> contour = computeRegularPolygon(nbContourPoints);
  _vertices.insert(_vertices.end(), contour.begin(), contour.end());
  for (unsigned int i = 0 ; i < nbContourPoints - 1 ; ++i) {
    _indices.push_back(0);
    _indices.push_back(i+1);
    _indices.push_back(i+2);
  }
  _indices.push_back(0);
  _indices.push_back(nbContourPoints);
  _indices.push_back(1);
  for (unsigned int i = 0 ; i < nbContourPoints - 1 ; ++i) {
    _outlineIndices.push_back(i+1);
    _outlineIndices.push_back(i+2);
  }
  _outlineIndices.push_back(nbContourPoints);
  _outlineIndices.push_back(1);
}

void CircleGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
  boundingBox[0] = Coord(-0.35f, -0.35f, 0);
  boundingBox[1] = Coord(0.35f, 0.35f, 0);
}
