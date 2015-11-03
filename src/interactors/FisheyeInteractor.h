#ifndef FISHEYEINTERACTOR_H
#define FISHEYEINTERACTOR_H

#include "GlSceneInteractor.h"
#include "GlScene.h"

#include <vector>

class ZoomAndPanInteractor;
class GlShaderProgram;
class GlBuffer;

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

    MouseButton _mouseButton;
    int _curX, _curY;
    bool _dragStarted;

    ZoomAndPanInteractor *_znpInteractor;

    GlShaderProgram *_fisheyeShader;
    GlBuffer *_buffer;
    GlBuffer *_indicesBuffer;

};

#endif // FISHEYEINTERACTOR_H
