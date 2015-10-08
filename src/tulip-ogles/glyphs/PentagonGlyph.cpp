#include "PentagonGlyph.h"

#include "../Utils.h"

using namespace std;
using namespace tlp;

PentagonGlyph::PentagonGlyph() {
    _vertices.push_back(Coord(0.f, 0.f));
    vector<Coord> contour = computeRegularPolygon(5, M_PI/2.f);
    _vertices.insert(_vertices.end(), contour.begin(), contour.end());
    for (unsigned int i = 0 ; i < 4 ; ++i) {
        _indices.push_back(0);
        _indices.push_back(i+1);
        _indices.push_back(i+2);
    }
    _indices.push_back(0);
    _indices.push_back(5);
    _indices.push_back(1);
    for (size_t i = 0 ; i < _vertices.size() - 2 ; ++i) {
      _outlineIndices.push_back(i+1);
      _outlineIndices.push_back(i+2);
    }
    _outlineIndices.push_back(_vertices.size()-1);
    _outlineIndices.push_back(1);
}

void PentagonGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
    boundingBox[0] = Coord(-0.3f, -0.35f, 0);
    boundingBox[1] = Coord(0.3f, 0.35f, 0);
}

