#include <GL/glew.h>

#include "NanoVGManager.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

std::string NanoVGManager::_currentCanvasId;
std::map<std::string, NanoVGManager *> NanoVGManager::_instances;

NanoVGManager::NanoVGManager() {
  _vg = nvgCreateGLES2(NVG_ANTIALIAS);
}

NanoVGManager::~NanoVGManager() {
  nvgDeleteGLES2(_vg);
}

NVGcontext* NanoVGManager::getNanoVGContext() const {
  return _vg;
}

NanoVGManager *NanoVGManager::instance() {
  return instance(_currentCanvasId);
}

NanoVGManager *NanoVGManager::instance(const std::string &canvasId) {
  if (_instances.find(canvasId) == _instances.end()) {
    _instances[canvasId] = new NanoVGManager();
  }
  return _instances[canvasId];
}
