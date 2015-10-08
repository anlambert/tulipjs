#include "CubeGlyph.h"

using namespace tlp;

CubeGlyph::CubeGlyph() {
    _vertices.push_back(Coord(0.5f, 0.5f, 0.5f));
    _vertices.push_back(Coord(-0.5f, 0.5f, 0.5f));
    _vertices.push_back(Coord(-0.5f, -0.5f, 0.5f));
    _vertices.push_back(Coord(0.5f, -0.5f, 0.5f));

    _vertices.push_back(Coord(0.5f, 0.5f, 0.5f));
    _vertices.push_back(Coord(0.5f, -0.5f, 0.5f));
    _vertices.push_back(Coord(0.5f, -0.5f, -0.5f));
    _vertices.push_back(Coord(0.5f, 0.5f, -0.5f));

    _vertices.push_back(Coord(0.5f, 0.5f, 0.5f));
    _vertices.push_back(Coord(0.5f, 0.5f, -0.5f));
    _vertices.push_back(Coord(-0.5f, 0.5f, -0.5f));
    _vertices.push_back(Coord(-0.5f, 0.5f, 0.5f));

    _vertices.push_back(Coord(-0.5f, 0.5f, 0.5f));
    _vertices.push_back(Coord(-0.5f, 0.5f, -0.5f));
    _vertices.push_back(Coord(-0.5f, -0.5f, -0.5f));
    _vertices.push_back(Coord(-0.5f, -0.5f, 0.5f));

    _vertices.push_back(Coord(-0.5f, -0.5f, -0.5f));
    _vertices.push_back(Coord(0.5f, -0.5f, -0.5f));
    _vertices.push_back(Coord(0.5f, -0.5f, 0.5f));
    _vertices.push_back(Coord(-0.5f, -0.5f, 0.5f));

    _vertices.push_back(Coord(0.5f, -0.5f, -0.5f));
    _vertices.push_back(Coord(-0.5f, -0.5f, -0.5f));
    _vertices.push_back(Coord(-0.5f, 0.5f, -0.5f));
    _vertices.push_back(Coord(0.5f, 0.5f, -0.5f));

    _texCoords.push_back(Vec2f(1.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 0.0f));
    _texCoords.push_back(Vec2f(1.0f, 0.0f));

    _texCoords.push_back(Vec2f(1.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 0.0f));
    _texCoords.push_back(Vec2f(1.0f, 0.0f));

    _texCoords.push_back(Vec2f(1.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 0.0f));
    _texCoords.push_back(Vec2f(1.0f, 0.0f));

    _texCoords.push_back(Vec2f(1.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 0.0f));
    _texCoords.push_back(Vec2f(1.0f, 0.0f));

    _texCoords.push_back(Vec2f(1.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 0.0f));
    _texCoords.push_back(Vec2f(1.0f, 0.0f));

    _texCoords.push_back(Vec2f(1.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 1.0f));
    _texCoords.push_back(Vec2f(0.0f, 0.0f));
    _texCoords.push_back(Vec2f(1.0f, 0.0f));

    _normals.push_back(Coord(0.0f, 0.0f, 1.0f));
    _normals.push_back(Coord(0.0f, 0.0f, 1.0f));
    _normals.push_back(Coord(0.0f, 0.0f, 1.0f));
    _normals.push_back(Coord(0.0f, 0.0f, 1.0f));

    _normals.push_back(Coord(1.0f, 0.0f, 0.0f));
    _normals.push_back(Coord(1.0f, 0.0f, 0.0f));
    _normals.push_back(Coord(1.0f, 0.0f, 0.0f));
    _normals.push_back(Coord(1.0f, 0.0f, 0.0f));

    _normals.push_back(Coord(0.0f, 1.0f, 0.0f));
    _normals.push_back(Coord(0.0f, 1.0f, 0.0f));
    _normals.push_back(Coord(0.0f, 1.0f, 0.0f));
    _normals.push_back(Coord(0.0f, 1.0f, 0.0f));

    _normals.push_back(Coord(-1.0f, 0.0f, 0.0f));
    _normals.push_back(Coord(-1.0f, 0.0f, 0.0f));
    _normals.push_back(Coord(-1.0f, 0.0f, 0.0f));
    _normals.push_back(Coord(-1.0f, 0.0f, 0.0f));

    _normals.push_back(Coord(0.0f, -1.0f, 0.0f));
    _normals.push_back(Coord(0.0f, -1.0f, 0.0f));
    _normals.push_back(Coord(0.0f, -1.0f, 0.0f));
    _normals.push_back(Coord(0.0f, -1.0f, 0.0f));

    _normals.push_back(Coord(0.0f, 0.0f, -1.0f));
    _normals.push_back(Coord(0.0f, 0.0f, -1.0f));
    _normals.push_back(Coord(0.0f, 0.0f, -1.0f));
    _normals.push_back(Coord(0.0f, 0.0f, -1.0f));

    for (size_t i = 0 ; i < _vertices.size() ; i+=4) {
        _indices.push_back(i);
        _indices.push_back(i+1);
        _indices.push_back(i+2);
        _indices.push_back(i);
        _indices.push_back(i+2);
        _indices.push_back(i+3);
    }

}

Coord CubeGlyph::getAnchor(const Coord & vector) const {
  float x, y, z, fmax;
  vector.get(x, y, z);
  fmax = std::max(std::max(fabsf(x), fabsf(y)), fabsf(z));

  if (fmax > 0.0f)
    return vector * (0.5f / fmax);
  else
    return vector;
}


