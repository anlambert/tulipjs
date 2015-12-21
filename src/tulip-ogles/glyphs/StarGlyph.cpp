#include "StarGlyph.h"

using namespace std;
using namespace tlp;

const float startAngle = static_cast<float>(M_PI)/2.0f;

vector<Coord> computeStar(const Coord &position, const Size &size, unsigned int numberOfStarPoints) {
  BoundingBox box;
  vector<Coord> points;
  float delta = (2.0f * M_PI) / static_cast<float>(numberOfStarPoints);

  for (unsigned int i = 0 ; i < numberOfStarPoints ; ++i) {
    float deltaX = cos(i * delta + startAngle);
    float deltaY = sin(i * delta + startAngle);
    points.push_back(Coord(deltaX,deltaY,0));
    box.expand(points.back());
    deltaX = 0.5f * cos(i * delta + delta/2.0f + startAngle);
    deltaY = 0.5f * sin(i * delta + delta/2.0f + startAngle);
    points.push_back(Coord(deltaX,deltaY,0));
    box.expand(points.back());
  }

  for(vector<Coord>::iterator it=points.begin(); it!=points.end(); ++it) {
    (*it)[0]=position[0]+(((*it)[0]-((box[1][0]+box[0][0])/2.))/((box[1][0]-box[0][0])/2.))*size[0];
    (*it)[1]=position[1]+(((*it)[1]-((box[1][1]+box[0][1])/2.))/((box[1][1]-box[0][1])/2.))*size[1];
  }

  return points;
}

StarGlyph::StarGlyph() {
  const unsigned int numberOfStarPoints = 5;
  _vertices = computeStar(Coord(0,0,0),Size(.5,.5,0),numberOfStarPoints);
  _vertices.insert(_vertices.begin(), Coord(0,0,0));
  for (unsigned int i = 0 ; i < 2*numberOfStarPoints - 1 ; ++i) {
    _indices.push_back(0);
    _indices.push_back(i+1);
    _indices.push_back(i+2);
  }
  _indices.push_back(0);
  _indices.push_back(10);
  _indices.push_back(1);
  for (size_t i = 0 ; i < _vertices.size() - 2 ; ++i) {
    _outlineIndices.push_back(i+1);
    _outlineIndices.push_back(i+2);
  }
  _outlineIndices.push_back(_vertices.size()-1);
  _outlineIndices.push_back(1);
}

void StarGlyph::getIncludeBoundingBox(BoundingBox &boundingBox) {
  boundingBox[0] = Coord(-0.3f, -0.35f, 0);
  boundingBox[1] = Coord(0.3f, 0.35f, 0);
}
