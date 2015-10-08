#ifndef SHADERS_H
#define SHADERS_H

#include <string>
#include <cfloat>
#include <map>

static std::string defaultVertexShaderSrc =
#ifdef __EMSCRIPTEN__
    "precision highp float;\n"
    "precision highp int;\n"
#else
    "#version 120\n"
#endif

    "uniform mat4 u_modelviewMatrix;"
    "uniform mat4 u_projectionMatrix;"
    "uniform vec4 u_color;"
    "uniform bool u_globalColor;"
    "uniform bool u_globalTexture;"
    "uniform bool u_textureActivated;"
    "uniform bool u_pointsRendering;"
    "uniform bool u_globalPointSize;"
    "uniform float u_pointSize;"
    "uniform vec4 u_texCoordOffsets;"

    "attribute vec3 a_position;"
    "attribute vec4 a_color;"
    "attribute vec2 a_texCoord;"
    "attribute float a_pointSize;"
    "attribute vec4 a_texCoordOffsets;"

    "varying vec4 v_color;"
    "varying vec2 v_texCoord;"
    "varying vec4 v_texCoordOffsets;"

    "void main() {"
    "  gl_Position = u_projectionMatrix * u_modelviewMatrix * vec4(a_position, 1.0);"
    "  if (u_globalColor) {"
    "    v_color = u_color;"
    "  } else {"
    "    v_color = a_color;"
    "  }"
    "  if (u_textureActivated) {"
    "    v_texCoord = a_texCoord;"
    "    if (u_globalTexture) {"
    "      v_texCoordOffsets = u_texCoordOffsets;"
    "    } else {"
    "      v_texCoordOffsets = a_texCoordOffsets;"
    "    }"
    "  }"

    "  if (u_pointsRendering) {"
    "    if (u_globalPointSize) {"
    "      gl_PointSize = u_pointSize;"
    "    } else {"
    "      gl_PointSize = a_pointSize;"
    "    }"
    "  }"
    "}"
    ;

static std::string defaultFragmentShaderSrc =
#ifdef __EMSCRIPTEN__
    "precision highp float;\n"
    "precision highp int;\n"
#else
    "#version 120\n"
#endif

    "uniform bool u_textureActivated;"
    "uniform sampler2D u_texture;"

    "varying vec4 v_color;"
    "varying vec2 v_texCoord;"
    "varying vec4 v_texCoordOffsets;"

    "void main() {"
    "  if (!u_textureActivated) {"
    "    gl_FragColor = v_color;"
    "  } else {"
    "    vec2 texCoord = vec2(v_texCoordOffsets.x + v_texCoord.x * (v_texCoordOffsets.z - v_texCoordOffsets.x),"
    "                         v_texCoordOffsets.y + v_texCoord.y * (v_texCoordOffsets.w - v_texCoordOffsets.y));"
    "    gl_FragColor = v_color * texture2D(u_texture, texCoord);"
    "  }"
    "}"
    ;

static std::string blinnPhongVertexShaderSrc =
#ifdef __EMSCRIPTEN__
    "precision highp float;\n"
    "precision highp int;\n"
#else
    "#version 120\n"
#endif

    "uniform mat4 u_modelviewMatrix;"
    "uniform mat4 u_projectionMatrix;"
    "uniform mat4 u_normalMatrix;"

    "uniform bool u_globalColor;"
    "uniform bool u_textureActivated;"

    "uniform vec3 u_eyePosition;"
    "uniform vec4 u_lightPosition;"
    "uniform float u_lightConstantAttenuation;"
    "uniform float u_lightLinearAttenuation;"
    "uniform float u_lightQuadraticAttenuation;"
    "uniform vec4 u_lightModelAmbientColor;"
    "uniform vec4 u_lightAmbientColor;"
    "uniform vec4 u_lightDiffuseColor;"

    "uniform vec4 u_materialAmbientColor;"
    "uniform vec4 u_materialDiffuseColor;"

    "attribute vec3 a_position;"
    "attribute vec4 a_color;"
    "attribute vec2 a_texCoord;"
    "attribute vec3 a_normal;"

    "varying vec4 v_diffuseColor;"
    "varying vec4 v_ambientGlobalColor;"
    "varying vec4 v_ambientColor;"

    "varying vec3 v_normal;"
    "varying vec3 v_lightDir;"
    "varying vec3 v_halfVector;"
    "varying float v_attenuation;"

    "varying vec2 v_texCoord;"

    "void main(){"
    "  v_normal = normalize(mat3(u_normalMatrix) * a_normal);"
    "  vec4 pos = u_modelviewMatrix * vec4(a_position, 1.0);"
    "  vec3 lightVec = u_lightPosition.xyz-pos.xyz;"
    "  if (u_lightPosition.w == 0.0) {"
    "    lightVec = -u_lightPosition.xyz;"
    "  }"

    "  v_lightDir = normalize(lightVec);"
    "  float dist = length(lightVec);"
    "  v_attenuation = 1.0 / (u_lightConstantAttenuation +"
    "                  u_lightLinearAttenuation * dist +"
    "                  u_lightQuadraticAttenuation * dist * dist);"

    "  v_halfVector = u_eyePosition + u_lightPosition.xyz;"
    "  v_halfVector = normalize(v_halfVector);"

    "  if (u_globalColor) {"
    "    v_diffuseColor = u_lightDiffuseColor * u_materialDiffuseColor;"
    "  } else {"
    "    v_diffuseColor = u_lightDiffuseColor * a_color;"
    "  }"

    "  v_ambientColor = u_lightAmbientColor * u_materialAmbientColor;"
    "  v_ambientGlobalColor = u_lightModelAmbientColor * u_materialAmbientColor;"

    "  gl_Position = u_projectionMatrix * pos;"

    "  if (u_textureActivated) {"
    "    v_texCoord = a_texCoord;"
    "  }"
    "}"
    ;

static std::string blinnPhongFragmentShaderSrc =
#ifdef __EMSCRIPTEN__
    "precision highp float;\n"
    "precision highp int;\n"
#else
    "#version 120\n"
#endif

    "uniform bool u_flatShading;"
    "uniform bool u_textureActivated;"
    "uniform sampler2D u_texture;"

    "uniform vec4 u_lightSpecularColor;"
    "uniform vec4 u_materialSpecularColor;"
    "uniform float u_materialShininess;"

    "varying vec4 v_diffuseColor;"
    "varying vec4 v_ambientGlobalColor;"
    "varying vec4 v_ambientColor;"

    "varying vec3 v_normal;"
    "varying vec3 v_lightDir;"
    "varying vec3 v_halfVector;"
    "varying float v_attenuation;"

    "varying vec2 v_texCoord;"

    "void main() {"
    "  if (u_flatShading) {"
    "     gl_FragColor = v_diffuseColor;"
    "     if (u_textureActivated) {"
    "       gl_FragColor *= texture2D(u_texture, v_texCoord);"
    "     }"
    "     return;"
    "  }"
    "  vec3 normal = normalize(v_normal);"
    "  vec4 color = v_ambientGlobalColor + v_ambientColor;"
    "  float NdotL = max(dot(normal,normalize(v_lightDir)),0.0);"
    "  if (NdotL > 0.0) {"
    "    color += v_attenuation * v_diffuseColor * NdotL;"
    "    vec3 halfV = normalize(v_halfVector);"
    "    float NdotHV = max(dot(normal, halfV), 0.0);"
    "    color += v_attenuation * u_materialSpecularColor * u_lightSpecularColor * pow(NdotHV, u_materialShininess);"
    "  }"

    "  if (u_textureActivated) {"
    "    color *= texture2D(u_texture, v_texCoord);"
    "  }"

    "  gl_FragColor = color;"
    "}"
    ;

class GlShaderProgram;

class ShaderManager {

public:

  static ShaderManager *getInstance(const std::string &canvasId);

  static ShaderManager *getInstance();

  static void setCurrentCanvasId(const std::string &canvasId) {
    _currentCanvasId = canvasId;
  }

  GlShaderProgram *getDefaultRenderingShader() const {
    return _defaultRenderingShader;
  }

  GlShaderProgram *getBlinnPhongRenderingShader() const {
    return _blinnPhongRenderingShader;
  }

private:

  ShaderManager();

  static std::map<std::string, ShaderManager *> _instances;
  static std::string _currentCanvasId;

  GlShaderProgram *_defaultRenderingShader;

  GlShaderProgram *_blinnPhongRenderingShader;

};

#endif // SHADERS_H
