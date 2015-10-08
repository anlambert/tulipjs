#ifndef GLFRAMEBUFFEROBJECT_H
#define GLFRAMEBUFFEROBJECT_H

#include <GL/gl.h>

class GlFrameBufferObject {

public:

  enum Attachment {
    NoAttachment,
    Depth,
    Stencil,
    CombinedDepthStencil
  };

  GlFrameBufferObject(int width, int height, Attachment attachement = Depth,
                      GLint textureMagFilter = GL_NEAREST, GLint textureMinFilter = GL_NEAREST,
                      GLint textureWrap = GL_CLAMP_TO_EDGE, bool generateMipmap = false);

  ~GlFrameBufferObject();

  int width() const {
    return _width;
  }

  int height() const {
    return _height;
  }

  void bind();

  void release();

  bool isValid() const {
    return _isValid;
  }

  GLuint texture() {
    return _texture;
  }

private:

  int _width, _height;
  GLuint _fboHandle;
  GLuint _texture;
  GLuint _attachmentRbo;
  bool _isValid;

};

#endif // GLFRAMEBUFFEROBJECT_H
