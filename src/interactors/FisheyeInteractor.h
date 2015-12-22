#ifndef FISHEYEINTERACTOR_H
#define FISHEYEINTERACTOR_H

#include "GlSceneInteractor.h"
#include "GlScene.h"

#include <vector>

class ZoomAndPanInteractor;
class GlShaderProgram;
class GlBuffer;
class GlFrameBufferObject;

class FisheyeInteractor : public GlSceneInteractor {

public:

  FisheyeInteractor(GlScene *scene = NULL);

  virtual void activate();

  virtual void desactivate();

  void setScene(GlScene *glScene);

  virtual bool mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers);

  virtual bool mouseMoveCallback(int x, int y, const int &modifiers);

  virtual void draw();

private:

  int _curX, _curY;

  ZoomAndPanInteractor *_znpInteractor;

  GlShaderProgram *_fisheyeShader;
  GlBuffer *_buffer;
  GlBuffer *_indicesBuffer;
  GlFrameBufferObject *_fbo;

  int _fisheyeRadius;
  float _fisheyeHeight;

  int _maxTextureSize;

};

#endif // FISHEYEINTERACTOR_H
