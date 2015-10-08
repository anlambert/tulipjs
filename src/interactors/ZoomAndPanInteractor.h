#ifndef ZOOMANDPANINTERACTOR_H
#define ZOOMANDPANINTERACTOR_H

#include "GlSceneInteractor.h"

class GlScene;

class ZoomAndPanInteractor : public GlSceneInteractor {

public:

    ZoomAndPanInteractor(GlScene *glScene = NULL);

    bool mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers);

    bool mouseMoveCallback(int x, int y, const int &modifiers);

    bool keyboardCallback(const std::string &key, const int &modifiers);

    int _mouseButton;
    int _lastX, _lastY;
    bool _dragStarted;

};

#endif // ZOOMANDPANINTERACTOR_H
