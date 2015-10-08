#include <tulip/PropertyTypes.h>

#include <emscripten/emscripten.h>

int EMSCRIPTEN_KEEPALIVE main(int /* argc */, char ** /* argv */) {

  tlp::initTypeSerializers();

  //emscripten_run_script("if (typeof window != 'undefined' && typeof window.tulip != 'undefined') { window.tulip.setLoaded(); }");

  //emscripten_run_script("if (typeof window != 'undefined' && typeof window.tulip != 'undefined' && typeof window.tulip.onload == 'function') { window.tulip.onload(); }");

  return 0;
}
