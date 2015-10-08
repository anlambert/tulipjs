#include "HexagonGlyph.h"

#include "../Utils.h"

using namespace std;
using namespace tlp;

HexagonGlyph::HexagonGlyph() {
    _vertices.push_back(Coord(0.f, 0.f));
    vector<Coord> contour = computeRegularPolygon(6, M_PI/2.f);
    _vertices.insert(_vertices.end(), contour.begin(), contour.end());
    for (unsigned int i = 0 ; i < 5 ; ++i) {
        _indices.push_back(0);
        _indices.push_back(i+1);
        _indices.push_back(i+2);
    }
    _indices.push_back(0);
    _indices.push_back(6);
    _indices.push_back(1);

    for (size_t i = 0 ; i < _vertices.size()-2 ; ++i) {
      _outlineIndices.push_back(i+1);
      _outlineIndices.push_back(i+2);
    }
    _outlineIndices.push_back(_vertices.size()-1);
    _outlineIndices.push_back(1);
}

void HexagonGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
    boundingBox[0] = Coord(-0.35f, -0.35f, 0);
    boundingBox[1] = Coord(0.35f, 0.35f, 0);
}


