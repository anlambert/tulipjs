#ifndef GLBOUNDINGBOXSCENEVISITOR_H
#define GLBOUNDINGBOXSCENEVISITOR_H

#include <tulip/BoundingBox.h>
#include "GlSceneVisitor.h"

class GlEntity;

class GlBoundingBoxSceneVisitor : public GlSceneVisitor {

public:

  GlBoundingBoxSceneVisitor() {}

  virtual void visit(GlEntity *entity);

  tlp::BoundingBox getBoundingBox() {
    return _boundingBox;
  }

private:

  tlp::BoundingBox _boundingBox;

};

#endif // GLLODSCENEVISITOR_H
