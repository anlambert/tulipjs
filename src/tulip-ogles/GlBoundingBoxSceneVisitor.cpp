#include "GlBoundingBoxSceneVisitor.h"
#include "GlEntity.h"

void GlBoundingBoxSceneVisitor::visit(GlEntity *entity) {
  if(entity->isVisible()) {
    tlp::BoundingBox bb=entity->getBoundingBox();

    if(bb.isValid()) {
      _boundingBox.expand(bb[0]);
      _boundingBox.expand(bb[1]);
    }
  }
}

