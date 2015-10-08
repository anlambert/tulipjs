#include "GlLODSceneVisitor.h"
#include "GlEntity.h"
#include "GlLODCalculator.h"

void GlLODSceneVisitor::visit(GlLayer *layer) {
  _layer = layer;
}

void GlLODSceneVisitor::visit(GlEntity *entity) {
  _calculator->addGlEntity(_layer, entity);
}

