#include "CubeOutlinedTransparentGlyph.h"

#include "CubeOutlinedGlyph.h"

using namespace tlp;
using namespace std;

CubeOutlinedTransparentGlyph::CubeOutlinedTransparentGlyph() {
  _cubeOutlinedGlyph = new CubeOutlinedGlyph();
}

CubeOutlinedTransparentGlyph::~CubeOutlinedTransparentGlyph() {
  delete _cubeOutlinedGlyph;
}

const vector<Coord> &CubeOutlinedTransparentGlyph::getGlyphVertices() const {
  return _cubeOutlinedGlyph->getGlyphVertices();
}

//const vector<Vec2f> &CubeOutlinedTransparentGlyph::getGlyphTexCoords() {
//  return _cubeOutlinedGlyph->getGlyphTexCoords();
//}

//const vector<Coord> &CubeOutlinedTransparentGlyph::getGlyphNormals() {
//  return _cubeOutlinedGlyph->getGlyphNormals();
//}

const vector<unsigned short> &CubeOutlinedTransparentGlyph::getGlyphOutlineIndices() const {
  return _cubeOutlinedGlyph->getGlyphOutlineIndices();
}

Coord CubeOutlinedTransparentGlyph::getAnchor(const Coord &vector) const {
  return _cubeOutlinedGlyph->getAnchor(vector);
}
