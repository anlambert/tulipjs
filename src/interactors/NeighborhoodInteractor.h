#ifndef NEIGHBORHOODINTERACTOR_H
#define NEIGHBORHOODINTERACTOR_H

#include <tulip/Graph.h>

#include "GlSceneInteractor.h"

class GlScene;
class GlGraph;
class ZoomAndPanInteractor;

class NeighborhoodInteractor : public GlSceneInteractor {

public:

  NeighborhoodInteractor(GlScene *scene = NULL);

  void setScene(GlScene *glScene);

  virtual bool mouseCallback(const MouseButton & button, const MouseButtonState &state, int x, int y, const int & modifiers);

  virtual bool mouseMoveCallback(int x, int y, const int & modifiers);

  virtual void draw();

private:

  void buildNeighborhoodSubgraph();
  void destroyNeighborhoodSubgraph();

  ZoomAndPanInteractor *_znpInteractor;

  GlGraph *_glGraph;
  tlp::node _centralNode;

  tlp::Graph *_neighborhoodSg;
  GlGraph *_glNeighborhoodGraph;
};

#endif // NEIGHBORHOODINTERACTOR_H
