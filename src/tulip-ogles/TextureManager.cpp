#include <GL/glew.h>

#include "TextureManager.h"
#include "Utils.h"

using namespace std;
using namespace tlp;

map<string, TextureManager *> TextureManager::_instances;
string TextureManager::_currentCanvasId("");

static int maxTextureSize = 0;

TextureManager::TextureManager() :
  _currentUnit(0) {
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
  if (maxTextureSize > 4096) {
    maxTextureSize = 4096;
  }
  for (int i = 0 ; i < 4 ; ++i) {
    _texturesAtlas[i] = NULL;
  }
}

TextureManager::~TextureManager() {
  for (int i = 0 ; i < 4 ; ++i) {
    delete _texturesAtlas[i];
  }
}

TextureManager *TextureManager::instance(const std::string &canvasId) {
  if (_instances.find(canvasId) == _instances.end()) {
    _instances[canvasId] = new TextureManager();
  }
  return _instances[canvasId];
}

TextureManager *TextureManager::instance() {
  return instance(_currentCanvasId);
}

static int nbTexturesInAtlas = 0;

void TextureManager::addTextureInAtlasFromFile(const std::string &textureFile) {

  if (textureFile.empty() || _textureAtlasUnit.find(textureFile) != _textureAtlasUnit.end()) {
    return;
  }
  TextureData *textureData = loadTextureData(textureFile.c_str());
  if (!textureData) {
    return;
  }
  addTextureInAtlasFromData(textureFile, textureData->pixels, textureData->width, textureData->height);
  delete textureData;
}

void TextureManager::addTextureInAtlasFromData(const std::string &textureName, const unsigned char *textureData, unsigned int width, unsigned int height) {
  if (textureName.empty() || _textureAtlasUnit.find(textureName) != _textureAtlasUnit.end()) {
    return;
  }
  bool ok = false;
  while (!ok && _currentUnit < 4) {
    if (!_texturesAtlas[_currentUnit]) {
      _texturesAtlas[_currentUnit] = new TextureAtlas(maxTextureSize, maxTextureSize, 4);
    }
    ivec4 region = _texturesAtlas[_currentUnit]->getRegion(width, height);
    if (region.x >= 0 && region.y >=0) {
      _texturesAtlas[_currentUnit]->setRegion(region.x, region.y, region.z, region.w, textureData, width*4);
      ++nbTexturesInAtlas;
      _textureAtlasUnit[textureName] = _currentUnit;
      float offset = 0;
      _coordinatesOffsets[textureName] = Vec4f((region.x+offset)/static_cast<float>(maxTextureSize), (region.y+offset)/static_cast<float>(maxTextureSize),
                                               (region.x+region.z-offset)/static_cast<float>(maxTextureSize), (region.y+region.w-offset)/static_cast<float>(maxTextureSize));
      ok = true;
    } else {
      std::cout << "texture atlas " <<  _currentUnit << " is full ! " << nbTexturesInAtlas << std::endl;
      ++_currentUnit;
      nbTexturesInAtlas = 0;
    }
  }
}

GLint TextureManager::getSamplerIdForTexture(const std::string &texture, bool forceUseAtlas) {
  GLint ret = -1;
  GLint retAtlas = -1;
  if (_textureUnit.find(texture) != _textureUnit.end()) {
    ret = _textureUnit[texture];
  }
  if (_textureAtlasUnit.find(texture) != _textureAtlasUnit.end()) {
    retAtlas = _textureAtlasUnit[texture];
  }
  if (ret >= 0) {
    if (retAtlas >= 0 && forceUseAtlas) {
      return retAtlas;
    } else {
      return ret;
    }
  } else {
    return retAtlas;
  }
}

Vec4f TextureManager::getCoordinatesOffsetsForTexture(const std::string &texture, bool forceUseAtlas) {
  Vec4f ret(0, 0, 1, 1);
  if (_textureUnit.find(texture) != _textureUnit.end()) {
    if (forceUseAtlas && _coordinatesOffsets.find(texture) != _coordinatesOffsets.end()) {
      ret = _coordinatesOffsets[texture];
    }
  } else {
    if (_coordinatesOffsets.find(texture) != _coordinatesOffsets.end()) {
      ret = _coordinatesOffsets[texture];
    }
  }
  return ret;
}

void TextureManager::bindTexturesAtlas() {

  if (_texturesAtlas[0]) {
    glActiveTexture(GL_TEXTURE0);
    _texturesAtlas[0]->bind();
  }
  if (_texturesAtlas[1]) {
    glActiveTexture(GL_TEXTURE1);
    _texturesAtlas[1]->bind();
  }
  if (_texturesAtlas[2]) {
    glActiveTexture(GL_TEXTURE2);
    _texturesAtlas[2]->bind();
  }
  if (_texturesAtlas[3]) {
    glActiveTexture(GL_TEXTURE3);
    _texturesAtlas[3]->bind();
  }
}

void TextureManager::unbindTexturesAtlas() {
  if (_texturesAtlas[3]) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  if (_texturesAtlas[2]) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  if (_texturesAtlas[1]) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  if (_texturesAtlas[0]) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}

void TextureManager::addTextureFromFile(const std::string &textureFile, bool addAlsoInAtlas) {
  if (textureFile.empty() || (!addAlsoInAtlas && _coordinatesOffsets.find(textureFile) != _coordinatesOffsets.end()) || _textures.find(textureFile) != _textures.end()) {
    return;
  }
  TextureData *textureData = loadTextureData(textureFile.c_str());
  if (!textureData) {
    return;
  }
  GLuint textureId;
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureData->width, textureData->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData->pixels);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  _textures[textureFile] = textureId;
  if (addAlsoInAtlas) {
    addTextureInAtlasFromData(textureFile, textureData->pixels, textureData->width, textureData->height);
  }
  delete textureData;
}

void TextureManager::addTextureFromData(const std::string &textureName, const unsigned char *textureData, unsigned int width, unsigned int height, bool addAlsoInAtlas) {
  if (textureName.empty() || (!addAlsoInAtlas && _coordinatesOffsets.find(textureName) != _coordinatesOffsets.end()) || _textures.find(textureName) != _textures.end()) {
    return;
  }

  bool canMipmap = nearestPOT(width) == width && nearestPOT(height) == height;

  GLuint textureId;
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  if (canMipmap) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  if (canMipmap) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  GLenum format = GL_RGBA;
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, textureData);
  if (canMipmap) {
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  _textures[textureName] = textureId;
  if (addAlsoInAtlas) {
    addTextureInAtlasFromData(textureName, textureData, width, height);
  }
}

void TextureManager::addExternalTexture(const string &textureId, const GLuint glTextureId) {
  _textures[textureId] = glTextureId;
}

void TextureManager::bindTexture(const std::string &textureId, const unsigned int textureUnit) {
  if (_textures.find(textureId) == _textures.end()) {
    return;
  }
  if (textureUnit == 0) {
    glActiveTexture(GL_TEXTURE0);
    _textureUnit[textureId] = textureUnit;
  } else if (textureUnit == 1) {
    glActiveTexture(GL_TEXTURE1);
    _textureUnit[textureId] = textureUnit;
  } else if (textureUnit == 2) {
    glActiveTexture(GL_TEXTURE2);
    _textureUnit[textureId] = textureUnit;
  } else if (textureUnit == 3) {
    glActiveTexture(GL_TEXTURE3);
    _textureUnit[textureId] = textureUnit;
  } else {
    glActiveTexture(GL_TEXTURE0);
    _textureUnit[textureId] = 0;
  }
  glBindTexture(GL_TEXTURE_2D, _textures[textureId]);
}

void TextureManager::unbindTexture(const std::string &textureId) {
  if (_textures.find(textureId) == _textures.end()) {
    return;
  }
  if (_textureUnit[textureId] == 0) {
    glActiveTexture(GL_TEXTURE0);
  } else if (_textureUnit[textureId] == 1) {
    glActiveTexture(GL_TEXTURE1);
  } else if (_textureUnit[textureId] == 2) {
    glActiveTexture(GL_TEXTURE2);
  } else {
    glActiveTexture(GL_TEXTURE3);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  _textureUnit.erase(textureId);
}

