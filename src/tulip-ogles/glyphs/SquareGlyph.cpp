#include "SquareGlyph.h"

#include "../Utils.h"

using namespace std;
using namespace tlp;

SquareGlyph::SquareGlyph() {
  _vertices = computeRegularPolygon(4, M_PI/4.f);
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

Coord SquareGlyph::getAnchor(const Coord &vector) const {
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
