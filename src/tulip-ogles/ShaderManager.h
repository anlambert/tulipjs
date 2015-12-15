#ifndef SHADERS_H
#define SHADERS_H

#include <string>
#include <cfloat>
#include <map>

class GlShaderProgram;

class ShaderManager {

public:

  static ShaderManager *getInstance(const std::string &canvasId);

  static ShaderManager *getInstance();

  static void setCurrentCanvasId(const std::string &canvasId) {
    _currentCanvasId = canvasId;
  }

  static std::string getFXAAFunctionsSource();

  GlShaderProgram *getFlatRenderingShader() const {
    return _flatRenderingShader;
  }

  GlShaderProgram *getBlinnPhongRenderingShader() const {
    return _blinnPhongRenderingShader;
  }

private:

  ShaderManager();

  static std::map<std::string, ShaderManager *> _instances;
  static std::string _currentCanvasId;

  GlShaderProgram *_flatRenderingShader;

  GlShaderProgram *_blinnPhongRenderingShader;

};

#endif // SHADERS_H
