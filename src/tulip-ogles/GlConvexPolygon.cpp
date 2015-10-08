#include <GL/glew.h>

#include "GlConvexPolygon.h"
#include "ShaderManager.h"
#include "GlShaderProgram.h"
#include "GlBuffer.h"
#include "Camera.h"
#include "Utils.h"
#include "TextureManager.h"

#include <tulip/Matrix.h>
#include <tulip/DrawingTools.h>

using namespace tlp;
using namespace std;

GlConvexPolygon::GlConvexPolygon() :
  _filled(true), _outlined(true), _fillColor(Color::Red), _outlineColor(Color::Black), _outlineWidth(1.f),
  _polygonDataBuffer(NULL), _polygonIndicesBuffer(NULL), _nbVertices(0), _nbIndices(0) {}

GlConvexPolygon::GlConvexPolygon(const std::vector<tlp::Coord> &contour, const tlp::Color &fillColor) :
  _filled(true), _outlined(false), _fillColor(fillColor), _outlineColor(Color::Black), _outlineWidth(1.f),
  _polygonDataBuffer(NULL), _polygonIndicesBuffer(NULL), _nbVertices(0), _nbIndices(0) {
  setPolygonContour(contour);
}

GlConvexPolygon::GlConvexPolygon(const std::vector<tlp::Coord> &contour, const tlp::Color &fillColor, const tlp::Color &outlineColor, const float outlineWidth) :
  _filled(true), _outlined(true), _fillColor(fillColor), _outlineColor(outlineColor), _outlineWidth(outlineWidth),
  _polygonDataBuffer(NULL), _polygonIndicesBuffer(NULL), _nbVertices(0), _nbIndices(0) {
  setPolygonContour(contour);
}

GlConvexPolygon::~GlConvexPolygon() {
  delete _polygonDataBuffer;
  delete _polygonIndicesBuffer;
}

void GlConvexPolygon::setPolygonContour(const std::vector<tlp::Coord> &contour) {
  assert(contour.size() > 2);

  _contour = contour;

  Mat3f invTransformMatrix;

  vector<Coord> polygonVertices;
  vector<unsigned short> polygonIndices;

  _boundingBox = BoundingBox();
  polygonVertices = contour;
  vector<Vec2f> projectedVertices;
  for(size_t i = 0 ; i < contour.size() ; ++i) {
    Coord p = Coord(invTransformMatrix * contour[i]);
    projectedVertices.push_back(Vec2f(p[0], p[1]));
    _boundingBox.expand(contour[i]);
  }

  if (clockwiseOrdering(projectedVertices)) {
    std::reverse(projectedVertices.begin(), projectedVertices.end());
    std::reverse(polygonVertices.begin(), polygonVertices.end());
  }
  assert(isConvexPolygon(projectedVertices));
  Coord centroid = computePolygonCentroid(polygonVertices);
  polygonVertices.insert(polygonVertices.begin(), centroid);

  for (size_t i = 0 ; i < contour.size()-1 ; ++i) {
    polygonIndices.push_back(0);
    polygonIndices.push_back(i+1);
    polygonIndices.push_back(i+2);
  }
  polygonIndices.push_back(0);
  polygonIndices.push_back(contour.size());
  polygonIndices.push_back(1);

  _nbVertices = polygonVertices.size();
  _nbIndices = polygonIndices.size();

  vector<float> polygonData;

  for(size_t i = 0 ; i < polygonVertices.size() ; ++i) {
    addTlpVecToVecFloat(polygonVertices[i], polygonData);
    addTlpVecToVecFloat(Vec2f((polygonVertices[i][0] - _boundingBox[0][0])/_boundingBox.width(), (polygonVertices[i][1] - _boundingBox[0][1])/_boundingBox.height()), polygonData);
  }

  if (!_polygonDataBuffer) {
    _polygonDataBuffer = new GlBuffer(GlBuffer::VertexBuffer);
    _polygonIndicesBuffer = new GlBuffer(GlBuffer::IndexBuffer);
  }

  _polygonDataBuffer->allocate(polygonData);
  _polygonIndicesBuffer->allocate(polygonIndices);

}

void GlConvexPolygon::draw(const Camera &camera, bool pickingMode) {
  GlEntity::draw(camera, pickingMode);
}

void GlConvexPolygon::draw(const Camera &camera, const Light &, bool pickingMode) {

  TextureManager::instance()->addTextureFromFile(_texture);
  TextureManager::instance()->bindTexture(_texture);

  GlShaderProgram *shader = ShaderManager::getInstance()->getDefaultRenderingShader();
  shader->activate();
  shader->setUniformMat4Float("u_projectionMatrix", camera.projectionMatrix());
  shader->setUniformMat4Float("u_modelviewMatrix", camera.modelviewMatrix());
  shader->setUniformBool("u_globalColor", true);
  shader->setUniformBool("u_pointsRendering", false);

  _polygonDataBuffer->bind();
  shader->setVertexAttribPointer("a_position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), BUFFER_OFFSET(0));

  if (!_texture.empty() && !pickingMode) {
    shader->setUniformBool("u_textureActivated", true);
    shader->setUniformBool("u_globalTexture", true);
    shader->setUniformTextureSampler("u_texture", TextureManager::instance()->getSamplerIdForTexture(_texture));
    shader->setUniformVec4Float("u_texCoordOffsets", TextureManager::instance()->getCoordinatesOffsetsForTexture(_texture));
    shader->setVertexAttribPointer("a_texCoord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
  } else {
    shader->setUniformBool("u_textureActivated", false);
  }

  if (_filled) {
    if (!pickingMode) {
      shader->setUniformColor("u_color", _fillColor);
    } else {
      shader->setUniformColor("u_color", _pickingColor);
    }
    _polygonIndicesBuffer->bind();
    glDrawElements(GL_TRIANGLES, _nbIndices, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
    _polygonIndicesBuffer->release();
  }

  shader->setUniformBool("u_textureActivated", false);

  if (_outlined && _outlineWidth > 0.f) {
    if (!pickingMode) {
      shader->setUniformColor("u_color", _outlineColor);
    } else {
      shader->setUniformColor("u_color", _pickingColor);
    }
    glLineWidth(_outlineWidth);
    glDrawArrays(GL_LINE_LOOP, 1, _nbVertices-1);
  }
  _polygonDataBuffer->release();

  TextureManager::instance()->unbindTexture(_texture);
}

void GlConvexPolygon::translate(const Coord &move) {
  for (size_t i = 0 ; i < _contour.size() ; ++i) {
    _contour[i] += move;
  }
  setPolygonContour(_contour);
  notifyModified();
}


