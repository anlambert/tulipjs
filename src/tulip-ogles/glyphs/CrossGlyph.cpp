#include "CrossGlyph.h"

using namespace tlp;

CrossGlyph::CrossGlyph() {
    float bWidth = 0.1f;
    _vertices.push_back(Coord(-bWidth, 0.5, 0));
    _vertices.push_back(Coord(bWidth, 0.5, 0));
    _vertices.push_back(Coord(bWidth, bWidth, 0));
    _vertices.push_back(Coord(0.5, bWidth, 0));
    _vertices.push_back(Coord(0.5, -bWidth, 0));
    _vertices.push_back(Coord(bWidth, -bWidth, 0));
    _vertices.push_back(Coord(bWidth, -0.5, 0));
    _vertices.push_back(Coord(-bWidth, -0.5, 0));
    _vertices.push_back(Coord(-bWidth, -bWidth, 0));
    _vertices.push_back(Coord(-0.5, -bWidth, 0));
    _vertices.push_back(Coord(-0.5, bWidth, 0));
    _vertices.push_back(Coord(-bWidth, bWidth, 0));

    _indices.push_back(2);
    _indices.push_back(1);
    _indices.push_back(0);

    _indices.push_back(0);
    _indices.push_back(11);
    _indices.push_back(2);

    _indices.push_back(10);
    _indices.push_back(9);
    _indices.push_back(3);

    _indices.push_back(9);
    _indices.push_back(4);
    _indices.push_back(3);

    _indices.push_back(8);
    _indices.push_back(7);
    _indices.push_back(6);

    _indices.push_back(8);
    _indices.push_back(6);
    _indices.push_back(5);

    for (size_t i = 0 ; i < _vertices.size() - 1 ; ++i) {
      _outlineIndices.push_back(i);
      _outlineIndices.push_back(i+1);
    }
    _outlineIndices.push_back(_vertices.size()-1);
    _outlineIndices.push_back(0);
}

Coord CrossGlyph::getAnchor(const Coord &vector) const {
    Coord v(vector);
    float x, y, z;
    v.get(x, y, z);
    // initialize anchor as the middle of segment points[0], points[1]
    Coord anchor(0, 0.5, 0);
    float distMin = x*x + ((y - 0.5) * (y - 0.5));
    // check with the middle of segment points[3], points[4]
    float dist = ((x - 0.5) * (x - 0.5)) + y*y;

    if (distMin > dist) {
      distMin = dist;
      anchor = Coord(0.5, 0, 0);
    }

    // check with the middle of segment points[7], points[6]
    dist = x*x + ((y + 0.5)*(y + 0.5));

    if (distMin > dist) {
      distMin = dist;
      anchor = Coord(0, -0.5, 0);
    }

    // check with the middle of segment points[9], points[10]
    if (distMin > ((x + 0.5) * (x + 0.5)) + y*y)
      return Coord(-0.5, 0, 0);

    return anchor;
}

void CrossGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
    boundingBox[0] = Coord(-0.5, -0.1);
    boundingBox[1] = Coord(0.5, 0.1);
}
