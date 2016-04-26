#ifndef CURVEUTILS_H
#define CURVEUTILS_H

#include <GL/gl.h>
#include <GL/glu.h>

#include <tulip/Coord.h>
#include <tulip/Size.h>
#include <tulip/Graph.h>
#include <tulip/Color.h>
#include <tulip/Matrix.h>
#include <tulip/BoundingBox.h>

#include <vector>
#include <sstream>

namespace tlp {
typedef Matrix<float, 4> MatrixGL;
class SizeProperty;
}

class GlGraphRenderingParameters;

template <unsigned int SIZE>
void addTlpVecToVecFloat(const tlp::Vector<float, SIZE> &c, std::vector<float> &v) {
  for (size_t i = 0 ; i < SIZE ; ++i) {
    v.push_back(c[i]);
  }
}

template <typename T>
std::string toString(const T &val) {
  std::ostringstream oss;
  oss << val;
  return oss.str();
}

void addColorToVecFloat(const tlp::Color &c, std::vector<float> &v);

void getSizes(const std::vector<tlp::Coord> &line, float s1, float s2, std::vector<float> &result);

void getColors(const std::vector<tlp::Coord> &line, const tlp::Color &c1, const tlp::Color &c2, std::vector<tlp::Color> &result);

void buildCurvePoints (const std::vector<tlp::Coord> &vertices, const std::vector<float> &sizes, const tlp::Coord &startN, const tlp::Coord &endN, std::vector<tlp::Coord> &result);

std::vector<tlp::Coord> computeRegularPolygon(unsigned int numberOfSides, float startAngle=0.f, const tlp::Coord &position = tlp::Coord(0.f,0.f), const tlp::Size &size = tlp::Size(0.5f, 0.5f));

bool clockwiseOrdering(const std::vector<tlp::Vec2f> &points);

float compute2DVectorsAngle(const tlp::Vec2f &a, const tlp::Vec2f &b);

bool isConvexPolygon(const std::vector<tlp::Vec2f> &vertices);

bool convexPolygonsIntersect(const std::vector<tlp::Vec2f> &convexPolygonA, const std::vector<tlp::Vec2f> &convexPolygonB);

unsigned int nearestPOT(unsigned int x);

struct TextureData {

  TextureData(unsigned int width, unsigned int height,
            unsigned int nbBytesPerPixel, unsigned char *pixels) :
    width(width), height(height), nbBytesPerPixel(nbBytesPerPixel), pixels(pixels) {}

  ~TextureData() {
    delete [] pixels;
  }

  unsigned int width;
  unsigned int height;
  unsigned int nbBytesPerPixel;
  unsigned char *pixels;

};

TextureData *loadTextureData(const char *file);

GLuint loadTexture(const char *file);

void setApplicationDirPath(const char *argv0);

std::string getApplicationDirPath();

std::string getGraphFileDirPath(tlp::Graph *graph);

tlp::Color uintToColor(const unsigned int n);

tlp::Color genRandomColor(const unsigned char alpha = 255);

unsigned int colorToUint(tlp::Color c);

tlp::Coord projectPoint(const tlp::Coord &obj, const tlp::MatrixGL &transform, const tlp::Vec4i &viewport);

tlp::Coord unprojectPoint(const tlp::Coord &obj, const tlp::MatrixGL &invtransform, const tlp::Vec4i &viewport);

float calculateAABBSize(const tlp::BoundingBox &bb, const tlp::Coord &eye,const tlp::MatrixGL &transformMatrix,
                        const tlp::Vec4i &globalViewport, const tlp::Vec4i &currentViewport);

float calculate2DLod(const tlp::BoundingBox &bb, const tlp::Vec4i& globalViewport, const tlp::Vec4i& currentViewport);

GLfloat projectSize(const tlp::Coord &position,const tlp::Size& size,
                    const tlp::MatrixGL &projectionMatrix, const tlp::MatrixGL &modelviewMatrix,
                    const tlp::Vec4i &viewport);

GLfloat projectSize(const tlp::BoundingBox &bb,
                    const tlp::MatrixGL &projectionMatrix, const tlp::MatrixGL &modelviewMatrix,
                    const tlp::Vec4i &viewport);

const std::string& glGetErrorDescription(GLuint errorCode);

void checkOGLError(const std::string &function, const std::string &file, const unsigned int line);

#define checkOpenGLError() checkOGLError(__PRETTY_FUNCTION__, __FILE__, __LINE__)

bool fileExists(const char *fileName);

void tokenize(const std::string &str, std::vector<std::string> &tokens, const std::string &delimiters);

tlp::Size getEdgeSize(tlp::Graph *graph, tlp::edge e, tlp::SizeProperty *viewSize, GlGraphRenderingParameters *renderingParameters);

std::vector<tlp::Coord> simplifyCurve(const std::vector<tlp::Coord> &in);

#endif // CURVEUTILS_H
