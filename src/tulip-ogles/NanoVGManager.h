#ifndef NANOVGMANAGER_H
#define NANOVGMANAGER_H

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include "nanovg.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include <map>
#include <string>

class NanoVGManager {

public:

  ~NanoVGManager();

  static NanoVGManager *instance();

  static NanoVGManager *instance(const std::string &canvasId);

  static void setCurrentCanvasId(const std::string &canvasId) {
    _currentCanvasId = canvasId;
  }

  NVGcontext* getNanoVGContext() const;

private:

  NanoVGManager();

  NVGcontext* _vg;

  static std::string _currentCanvasId;
  static std::map<std::string, NanoVGManager *> _instances;

};

#endif // NANOVGMANAGER_H
