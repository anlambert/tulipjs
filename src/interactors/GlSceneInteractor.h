#ifndef GLUTINTERACTOR_H
#define GLUTINTERACTOR_H

#include <string>

extern void timerFunc(unsigned int msecs, void (*func)(void *value), void *value);

class GlScene;

enum SpecialKey {
  KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
  KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
  KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_PAGE_UP,
  KEY_PAGE_DOWN, KEY_HOME, KEY_END, KEY_INSERT
};

enum MouseButton {
  LEFT_BUTTON, MIDDLE_BUTTON, RIGHT_BUTTON, WHEEL
};

enum MouseButtonState {
  DOWN, UP
};

enum MouseEntryState {
  LEFT, ENTERED
};

enum KeyboardModifiers {
  ACTIVE_SHIFT = 1, ACTIVE_CTRL = 2, ACTIVE_ALT = 4
};

class GlSceneInteractor {

public:

    virtual void activate() {}

    virtual void desactivate() {}

    virtual bool mouseCallback(const MouseButton & /*button*/, const MouseButtonState & /*state*/, int /*x*/, int /*y*/, const int & /*modifiers*/) {return false;}

    virtual bool mouseMoveCallback(int /*x*/, int /*y*/, const int & /*modifiers*/) {return false;}

    virtual bool keyboardCallback(const std::string& /*keyStr*/, const int & /*modifiers*/) {return false;}

    virtual void draw() {}

    virtual void setScene(GlScene *glScene) {
      _glScene = glScene;
    }

protected:

    GlScene *_glScene;

};

#endif // GLUTINTERACTOR_H
