#ifndef GLLODSCENEVISITOR_H
#define GLLODSCENEVISITOR_H

#include "GlSceneVisitor.h"

class GlLODCalculator;

class GlLODSceneVisitor : public GlSceneVisitor {

public:

  GlLODSceneVisitor(GlLODCalculator *calculator) : _layer(0), _calculator(calculator) {}

  virtual void visit(GlLayer *);
  void visit(GlEntity *entity);

private:

  GlLayer *_layer;
  GlLODCalculator* _calculator;

};

#endif // GLLODSCENEVISITOR_H
