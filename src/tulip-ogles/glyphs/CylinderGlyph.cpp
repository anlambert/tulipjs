#include "CylinderGlyph.h"

#include "../Utils.h"

using namespace std;
using namespace tlp;

static const unsigned int nbContourPoints = 30;

static void genCylinderGlyph(const float zBottom, const float zTop, vector<Coord> &vertices, vector<unsigned short> &indices, vector<Vec2f> &texCoords) {
  vector<Coord> bottomCircle = computeRegularPolygon(nbContourPoints, 0.f, Coord(0.f, 0.f, zBottom));
  vector<Coord> topCircle = computeRegularPolygon(nbContourPoints, 0.f, Coord(0.f, 0.f, zTop));
  vertices.push_back(Coord(0.f, 0.f, zBottom));
  vertices.insert(vertices.end(), bottomCircle.begin(), bottomCircle.end());
  vertices.push_back(Coord(0.f, 0.f, zTop));
  vertices.insert(vertices.end(), topCircle.begin(), topCircle.end());

  for (unsigned int i = 0 ; i < nbContourPoints - 1 ; ++i) {
      indices.push_back(0);
      indices.push_back(i+1);
      indices.push_back(i+2);
  }
  indices.push_back(0);
  indices.push_back(nbContourPoints);
  indices.push_back(1);

  for (unsigned int i = 0 ; i < nbContourPoints - 1 ; ++i) {
      indices.push_back(nbContourPoints+1);
      indices.push_back(nbContourPoints+1+i+1);
      indices.push_back(nbContourPoints+1+i+2);
  }
  indices.push_back(nbContourPoints+1);
  indices.push_back(nbContourPoints+1+nbContourPoints);
  indices.push_back(nbContourPoints+1+1);

  for (size_t i = 0 ; i < vertices.size() ; ++i) {
      tlp::Vec2f st;
      st[0] = vertices[i][0] + 0.5f;
      st[1] = vertices[i][1] + 0.5f;
      texCoords.push_back(st);
  }

  unsigned startIdx = vertices.size();

  vertices.insert(vertices.end(), bottomCircle.begin(), bottomCircle.end());
  vertices.insert(vertices.end(), topCircle.begin(), topCircle.end());

  for (unsigned int i = 0 ; i < nbContourPoints - 1 ; ++i) {
      indices.push_back(startIdx+i);
      indices.push_back(startIdx+i+1);
      indices.push_back(startIdx+i+nbContourPoints);

      indices.push_back(startIdx+i+1);
      indices.push_back(startIdx+i+nbContourPoints+1);
      indices.push_back(startIdx+i+nbContourPoints);
  }

  indices.push_back(startIdx+nbContourPoints-1);
  indices.push_back(startIdx);
  indices.push_back(startIdx+2*nbContourPoints-1);

  indices.push_back(startIdx);
  indices.push_back(startIdx+nbContourPoints);
  indices.push_back(startIdx+2*nbContourPoints-1);

  for (size_t i = startIdx ; i < vertices.size() ; ++i) {
      tlp::Vec2f st;
      st[0] = vertices[i][0] + 0.5f;
      st[1] = vertices[i][2] + 0.5f;
      texCoords.push_back(st);
  }
}

CylinderGlyph::CylinderGlyph() {
  genCylinderGlyph(-0.5f, 0.5f, _vertices, _indices, _texCoords);
}

HalfCylinderGlyph::HalfCylinderGlyph() {
  genCylinderGlyph(0.f, 0.5f, _vertices, _indices, _texCoords);
}
