#include <tulip/PropertyTypes.h>

#include <emscripten/emscripten.h>

int EMSCRIPTEN_KEEPALIVE main(int /* argc */, char ** /* argv */) {

  tlp::initTypeSerializers();

  emscripten_run_script("if (typeof window != 'undefined' && typeof window.tulip != 'undefined') { window.tulip.mainCalled = true; }");

  emscripten_run_script("if (typeof tulip != 'undefined' && tulip.workerMode) { self.postMessage({eventType : 'tulipWorkerInit'}); }");

  return 0;
}
