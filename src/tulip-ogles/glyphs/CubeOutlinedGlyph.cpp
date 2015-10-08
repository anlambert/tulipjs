#include "CubeOutlinedGlyph.h"

#include "CubeGlyph.h"

using namespace tlp;
using namespace std;

CubeOutlinedGlyph::CubeOutlinedGlyph() {
  _cubeGlyph = new CubeGlyph();
  _outlineIndices.push_back(0);
  _outlineIndices.push_back(1);
  _outlineIndices.push_back(1);
  _outlineIndices.push_back(2);
  _outlineIndices.push_back(2);
  _outlineIndices.push_back(3);
  _outlineIndices.push_back(3);
  _outlineIndices.push_back(0);
  _outlineIndices.push_back(20);
  _outlineIndices.push_back(21);
  _outlineIndices.push_back(21);
  _outlineIndices.push_back(22);
  _outlineIndices.push_back(22);
  _outlineIndices.push_back(23);
  _outlineIndices.push_back(23);
  _outlineIndices.push_back(20);
  _outlineIndices.push_back(0);
  _outlineIndices.push_back(23);
  _outlineIndices.push_back(1);
  _outlineIndices.push_back(22);
  _outlineIndices.push_back(2);
  _outlineIndices.push_back(21);
  _outlineIndices.push_back(3);
  _outlineIndices.push_back(20);
}

CubeOutlinedGlyph::~CubeOutlinedGlyph() {
  delete _cubeGlyph;
}

void CubeOutlinedGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
  _cubeGlyph->getIncludeBoundingBox(boundingBox);
}

void CubeOutlinedGlyph::getTextBoundingBox(BoundingBox &boundingBox) {
  _cubeGlyph->getTextBoundingBox(boundingBox);
}

const vector<Coord> &CubeOutlinedGlyph::getGlyphVertices() const {
  return _cubeGlyph->getGlyphVertices();
}

const vector<Vec2f> &CubeOutlinedGlyph::getGlyphTexCoords() {
  return _cubeGlyph->getGlyphTexCoords();
}

const vector<Coord> &CubeOutlinedGlyph::getGlyphNormals() {
  return _cubeGlyph->getGlyphNormals();
}

const vector<unsigned short> &CubeOutlinedGlyph::getGlyphVerticesIndices() const {
  return _cubeGlyph->getGlyphVerticesIndices();
}

Coord CubeOutlinedGlyph::getAnchor(const Coord &vector) const {
  return _cubeGlyph->getAnchor(vector);
}
