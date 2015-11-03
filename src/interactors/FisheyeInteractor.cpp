#include "FisheyeInteractor.h"

#include "ZoomAndPanInteractor.h"
#include "GlShaderProgram.h"
#include "GlBuffer.h"
#include "Utils.h"
#include "Camera.h"
#include "TextureManager.h"

static std::string fisheyeVertexShaderSrc =
#ifdef __EMSCRIPTEN__
    "precision highp float;\n"
    "precision highp int;\n"
#else
    "#version 120\n"
#endif

    "uniform mat4 u_modelviewMatrix;"
    "uniform mat4 u_projectionMatrix;"

    "attribute vec3 a_position;"
    "attribute vec2 a_texCoord;"

    "varying vec2 v_texCoord;"

    "void main() {"
    "  gl_Position = u_projectionMatrix * u_modelviewMatrix * vec4(a_position, 1.0);"
    "  v_texCoord = a_texCoord;"
    "}"
    ;

static std::string fisheyeFragmentShaderSrc =
#ifdef __EMSCRIPTEN__
    "precision highp float;\n"
    "precision highp int;\n"
#else
    "#version 120\n"
#endif

    "uniform sampler2D u_texture;"
    "uniform vec2 u_mouse;"
    "uniform vec2 u_resolution;"

    "varying vec2 v_texCoord;"

    "const float radius = 200.0;"
    "const float height = -0.5;"

    "void main(void) {"
    "  vec2 pos = gl_FragCoord.xy;"
    "  vec2 center = u_mouse;"
    "  float dist = distance(center, pos);"

    "	if (dist < radius) {"
    "		float coeff = (height + 1.) * dist / (height * dist/ radius + 1.);"
    "		vec2 dir = normalize(pos - center) * coeff;"
    "		pos = center + dir;"
    "	}"
    "  gl_FragColor = texture2D(u_texture, pos / u_resolution);"
    "}"

;

FisheyeInteractor::FisheyeInteractor(GlScene *scene) :
  _curX(-1), _curY(-1), _dragStarted(false), _znpInteractor(NULL),
  _fisheyeShader(NULL), _buffer(NULL) {
  _glScene = scene;
  _znpInteractor = new ZoomAndPanInteractor(scene);
}

void FisheyeInteractor::activate() {
  _fisheyeShader = new GlShaderProgram();
  _fisheyeShader->addShaderFromSourceCode(GlShader::Vertex, fisheyeVertexShaderSrc);
  _fisheyeShader->addShaderFromSourceCode(GlShader::Fragment, fisheyeFragmentShaderSrc);
  _fisheyeShader->link();
  _fisheyeShader->printInfoLog();
  _buffer = new GlBuffer(GlBuffer::VertexBuffer);
  _indicesBuffer = new GlBuffer(GlBuffer::IndexBuffer);
}

void FisheyeInteractor::desactivate() {
  delete _fisheyeShader;
  delete _buffer;
  delete _indicesBuffer;
}

void FisheyeInteractor::setScene(GlScene *glScene) {
  GlSceneInteractor::setScene(glScene);
  _znpInteractor->setScene(glScene);
}

bool FisheyeInteractor::mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers) {
  if (!_glScene) return false;
  if (button == WHEEL) {
    return _znpInteractor->mouseCallback(button, state, x, y, modifiers);
  } else {
    _curX = x;
    _curY = y;
    _glScene->requestDraw();
  }
}

bool FisheyeInteractor::mouseMoveCallback(int x, int y, const int &modifiers) {
  if (!_glScene) return false;
  _curX = x;
  _curY = y;
  _glScene->requestDraw();
}

void FisheyeInteractor::draw() {

  Camera camera2d(false);
  tlp::Vec4i viewport = _glScene->getViewport();
  camera2d.setViewport(viewport);
  camera2d.initGl();
  std::vector<float> quadData;
  addTlpVecToVecFloat(tlp::Vec3f(0, 0, 0), quadData);
  addTlpVecToVecFloat(tlp::Vec2f(0, 0), quadData);
  addTlpVecToVecFloat(tlp::Vec3f(0, viewport[3], 0), quadData);
  addTlpVecToVecFloat(tlp::Vec2f(0, 1), quadData);
  addTlpVecToVecFloat(tlp::Vec3f(viewport[2], viewport[3], 0), quadData);
  addTlpVecToVecFloat(tlp::Vec2f(1, 1), quadData);
  addTlpVecToVecFloat(tlp::Vec3f(viewport[2], 0, 0), quadData);
  addTlpVecToVecFloat(tlp::Vec2f(1, 0), quadData);
  std::vector<unsigned short> indices = {0, 1, 2, 0, 2, 3};

  _buffer->bind();
  _buffer->allocate(quadData);
  _fisheyeShader->activate();
  _fisheyeShader->setUniformMat4Float("u_modelviewMatrix", camera2d.modelviewMatrix());
  _fisheyeShader->setUniformMat4Float("u_projectionMatrix", camera2d.projectionMatrix());
  _fisheyeShader->setVertexAttribPointer("a_position", 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), BUFFER_OFFSET(0));
  _fisheyeShader->setVertexAttribPointer("a_texCoord", 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), BUFFER_OFFSET(3*sizeof(float)));
  _fisheyeShader->setUniformTextureSampler("u_texture", 0);
  _fisheyeShader->setUniformVec2Float("u_mouse", _curX, viewport[3] - _curY);
  _fisheyeShader->setUniformVec2Float("u_resolution", viewport[2], viewport[3]);
  _indicesBuffer->bind();
  _indicesBuffer->allocate(indices);
  TextureManager::instance()->bindTexture(_glScene->getBackBufferTextureName());
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
  TextureManager::instance()->unbindTexture(_glScene->getBackBufferTextureName());
  _fisheyeShader->desactivate();
  _indicesBuffer->release();
  _buffer->release();


}
