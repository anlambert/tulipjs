#ifndef LASSOSELECTIONINTERACTOR_H
#define LASSOSELECTIONINTERACTOR_H

#include "GlSceneInteractor.h"
#include "GlScene.h"

#include <vector>

class ZoomAndPanInteractor;

class LassoSelectionInteractor : public GlSceneInteractor {

public:

  LassoSelectionInteractor(GlScene *scene = NULL);

  void setScene(GlScene *glScene);

  virtual bool mouseCallback(const MouseButton & button, const MouseButtonState &state, int x, int y, const int & modifiers);

  virtual bool mouseMoveCallback(int x, int y, const int & modifiers);

  virtual void draw();

private:

  void selectGraphElementsUnderPolygon();

  ZoomAndPanInteractor *_znpInteractor;

  bool _dragStarted;
  std::vector<tlp::Coord> _polygon;

};

#endif // LASSOSELECTIONINTERACTOR_H
