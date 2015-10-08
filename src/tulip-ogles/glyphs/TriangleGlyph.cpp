#include "TriangleGlyph.h"

#include "../Utils.h"

using namespace tlp;

TriangleGlyph::TriangleGlyph() {
    _vertices = computeRegularPolygon(3, M_PI/2.f);
    _indices.push_back(0);
    _indices.push_back(1);
    _indices.push_back(2);
    _outlineIndices.push_back(0);
    _outlineIndices.push_back(1);
    _outlineIndices.push_back(1);
    _outlineIndices.push_back(2);
    _outlineIndices.push_back(2);
    _outlineIndices.push_back(0);
}

void TriangleGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
    boundingBox[0] = Coord(-0.25,-0.5,0);
    boundingBox[1] = Coord(0.25,0,0);
}
