#include "FisheyeInteractor.h"

#include "ZoomAndPanInteractor.h"
#include "GlShaderProgram.h"
#include "GlBuffer.h"
#include "Utils.h"
#include "Camera.h"
#include "TextureManager.h"
#include "GlFrameBufferObject.h"
#include "GlLayer.h"
#include "ZoomAndPanAnimation.h"
#include "ShaderManager.h"

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
    "uniform sampler2D u_fisheyeTexture;"

    "uniform float u_fisheyeRadius;"
    "uniform float u_fisheyeHeight;"
    "uniform float u_fisheyeTextureSize;"

    "uniform vec2 u_mouse;"
    "uniform vec2 u_resolution;"

    "varying vec2 v_texCoord;"

    +

    ShaderManager::getFXAAFunctionsSource()

    +

    "void main(void) {"
    "  vec2 pos = v_texCoord * u_resolution;"
    "  vec2 center = u_mouse;"
    "  float radius = u_fisheyeRadius;"
    "  float height = u_fisheyeHeight;"
    "  float dist = distance(center, pos);"
    "  if (dist < radius && u_fisheyeHeight < -0.05) {"
    "		 float coeff = (height + 1.0) * dist / (height * dist/ radius + 1.0);"
    "		 vec2 dir = normalize(pos - center) * coeff;"
    "		 pos = center + dir;"
    "    vec2 fisheyePos = vec2(pos.x - (u_mouse.x - u_fisheyeRadius), pos.y - (u_mouse.y - u_fisheyeRadius)) / vec2(2.0*u_fisheyeRadius);"
    "    vec2 fisheyeTexRes = vec2(u_fisheyeTextureSize);"
    "    vec2 fisheyeFragCoord = fisheyePos * fisheyeTexRes;"
    "    gl_FragColor = applyFXAA(u_fisheyeTexture, fisheyeFragCoord, fisheyeTexRes);"
    "  } else {"
    "    gl_FragColor = texture2D(u_texture, pos / u_resolution);"
    "  }"
    "}"

    ;

FisheyeInteractor::FisheyeInteractor(GlScene *scene) :
  _curX(-1), _curY(-1), _dragStarted(false), _znpInteractor(NULL),
  _fisheyeShader(NULL), _buffer(NULL), _fbo(NULL), _fisheyeRadius(200), _fisheyeHeight(0.5), _maxTextureSize(0) {
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
  _fbo = NULL;

  if (_maxTextureSize == 0) {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_maxTextureSize);
    if (_maxTextureSize > 4096) {
      _maxTextureSize = 4096;
    }
  }

}

void FisheyeInteractor::desactivate() {
  delete _fisheyeShader;
  delete _buffer;
  delete _indicesBuffer;
  delete _fbo;
}

void FisheyeInteractor::setScene(GlScene *glScene) {
  GlSceneInteractor::setScene(glScene);
  _znpInteractor->setScene(glScene);
}

static float clamp(float n, float lower, float upper) {
  return std::max(lower, std::min(n, upper));
}

bool FisheyeInteractor::mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers) {
  if (!_glScene) return false;
  if (button == WHEEL) {
    if (modifiers == ACTIVE_CTRL) {
      if (state == UP) {
        _fisheyeRadius += 10;
      } else {
        _fisheyeRadius -= 10;
      }
      if (_fisheyeRadius < 0) _fisheyeRadius = 0;
      _glScene->requestDraw();
      return true;
    } else if (modifiers == ACTIVE_SHIFT) {
      if (state == UP) {
        _fisheyeHeight += 0.05;
      } else {
        _fisheyeHeight -= 0.05;
      }
      _fisheyeHeight = clamp(_fisheyeHeight, 0.0, 0.9);
      _glScene->requestDraw();
      return true;
    } else {
      return _znpInteractor->mouseCallback(button, state, x, y, modifiers);
    }
  } else {
    _curX = x;
    _curY = y;
    _glScene->requestDraw();
    return true;
  }
}

bool FisheyeInteractor::mouseMoveCallback(int x, int y, const int &modifiers) {
  if (!_glScene) return false;
  _curX = x;
  _curY = y;
  _glScene->requestDraw();
  return true;
}

void FisheyeInteractor::draw() {

  tlp::Vec4i viewport = _glScene->getViewport();

  int factor = 1;
  if (_fisheyeHeight > 0.1) {
    factor = _fisheyeHeight * 10;
  }
  int fboSize = factor * 2 * _fisheyeRadius;
  fboSize = std::min(fboSize, _maxTextureSize);

  if (_fbo == NULL || _fbo->width() != fboSize) {
    delete _fbo;
    _fbo = new GlFrameBufferObject(fboSize, fboSize, GlFrameBufferObject::CombinedDepthStencil, GL_LINEAR, GL_LINEAR);
    TextureManager::instance()->addExternalTexture("fisheyeTexture", _fbo->texture());
  }

  Camera *camera = _glScene->getMainLayer()->getCamera();
  Camera camBackup = *camera;

  tlp::Coord bbMin = camera->screenTo3DWorld(tlp::Coord(viewport[2] - (_curX - _fisheyeRadius), _curY - _fisheyeRadius));
  tlp::Coord bbMax = camera->screenTo3DWorld(tlp::Coord(viewport[2] - (_curX + _fisheyeRadius), _curY + _fisheyeRadius));
  tlp::BoundingBox bb;
  bb.expand(bbMin);
  bb.expand(bbMax);

  _fbo->bind();
  _glScene->setViewport(0, 0, fboSize, fboSize);
  adjustViewToBoundingBox(camera, bb);
  glDisable(GL_SCISSOR_TEST);
  _glScene->draw();
  _fbo->release();

  _glScene->setViewport(viewport);
  *camera = camBackup;
  _glScene->initGlParameters();
  _glScene->setSceneNeedRedraw(false);

  Camera camera2d(false);
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

  glDisable(GL_BLEND);

  _buffer->bind();
  _buffer->allocate(quadData);
  _fisheyeShader->activate();
  _fisheyeShader->setUniformMat4Float("u_modelviewMatrix", camera2d.modelviewMatrix());
  _fisheyeShader->setUniformMat4Float("u_projectionMatrix", camera2d.projectionMatrix());
  _fisheyeShader->setVertexAttribPointer("a_position", 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), BUFFER_OFFSET(0));
  _fisheyeShader->setVertexAttribPointer("a_texCoord", 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), BUFFER_OFFSET(3*sizeof(float)));
  _fisheyeShader->setUniformTextureSampler("u_texture", 0);
  _fisheyeShader->setUniformTextureSampler("u_fisheyeTexture", 1);
  _fisheyeShader->setUniformVec2Float("u_mouse", _curX, viewport[3] - _curY);
  _fisheyeShader->setUniformVec2Float("u_resolution", viewport[2], viewport[3]);
  _fisheyeShader->setUniformFloat("u_fisheyeRadius", _fisheyeRadius);
  _fisheyeShader->setUniformFloat("u_fisheyeHeight", -_fisheyeHeight);
  _fisheyeShader->setUniformFloat("u_fisheyeTextureSize", fboSize);
  _indicesBuffer->bind();
  _indicesBuffer->allocate(indices);
  TextureManager::instance()->bindTexture(_glScene->getBackBufferTextureName());
  TextureManager::instance()->bindTexture("fisheyeTexture", 1);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
  TextureManager::instance()->unbindTexture(_glScene->getBackBufferTextureName());
  TextureManager::instance()->unbindTexture("fisheyeTexture");
  _fisheyeShader->desactivate();
  _indicesBuffer->release();
  _buffer->release();


}
