#ifndef RECTANGLEZOOMINTERACTOR_H
#define RECTANGLEZOOMINTERACTOR_H

#include "GlSceneInteractor.h"

class GlScene;

struct AnimateParams {

  AnimateParams() : scene(NULL) {}
  GlScene *scene;
};

class RectangleZoomInteractor : public GlSceneInteractor {

public:

    RectangleZoomInteractor(GlScene *glScene = NULL);

    virtual bool mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers);

    virtual bool mouseMoveCallback(int x, int y, const int &modifiers);

    virtual bool keyboardCallback(const std::string &keyStr, const int &modifiers);

    virtual void draw();

private:

    MouseButton _mouseButton;
    int _firstX, _firstY;
    int _curX, _curY;
    bool _dragStarted;
    AnimateParams _animParams;
};

#endif // RECTANGLEZOOMINTERACTOR_H
