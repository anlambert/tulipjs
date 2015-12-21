#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <GL/gl.h>

#include <tulip/Vector.h>

#include <TextureAtlas.h>

#include <map>
#include <vector>

class TextureManager {

public:

  static TextureManager *instance(const std::string &canvasId);

  static TextureManager *instance();

  static void setCurrentCanvasId(const std::string &canvasId) {
    _currentCanvasId = canvasId;
  }

  ~TextureManager();

  void addTextureInAtlasFromFile(const std::string &textureFile);
  void addTextureInAtlasFromData(const std::string &textureName, const unsigned char *textureData, unsigned int width, unsigned int height);
  void bindTexturesAtlas();
  void unbindTexturesAtlas();

  void addTextureFromFile(const std::string &textureFile, bool addAlsoInAtlas=false);
  void addTextureFromData(const std::string &textureName, const unsigned char *textureData, unsigned int width, unsigned int height, bool addAlsoInAtlas=false);
  void addExternalTexture(const std::string &textureId, const GLuint glTextureId);
  void bindTexture(const std::string &textureId, const unsigned int textureUnit=0);
  void unbindTexture(const std::string &textureId);

  GLint getSamplerIdForTexture(const std::string &texture, bool forceUseAtlas=false);
  tlp::Vec4f getCoordinatesOffsetsForTexture(const std::string &texture, bool forceUseAtlas=false);

private:

  static std::map<std::string, TextureManager *> _instances;
  static std::string _currentCanvasId;

  TextureManager();

  TextureAtlas *_texturesAtlas[4];
  GLuint _currentUnit;

  std::map<std::string, GLuint> _textureUnit;
  std::map<std::string, GLuint> _textureAtlasUnit;
  std::map<std::string, tlp::Vec4f> _coordinatesOffsets;

  std::map<std::string, GLuint> _textures;


};

#endif // TEXTUREMANAGER_H
