#include <tulip/PropertyTypes.h>

#include <emscripten/emscripten.h>

int EMSCRIPTEN_KEEPALIVE main(int /* argc */, char ** /* argv */) {

  tlp::initTypeSerializers();

  emscripten_run_script("if (typeof window != 'undefined' && typeof window.tulip != 'undefined') { window.tulip.mainCalled = true; }");

  return 0;
}
