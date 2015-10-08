#ifndef Tulip_GLLODCALCULATOR_H
#define Tulip_GLLODCALCULATOR_H

#include <tulip/BoundingBox.h>
#include <tulip/Vector.h>
#include <tulip/Node.h>
#include <tulip/Edge.h>

#include <vector>
#include <map>

#include "GlEntity.h"

class Camera;
class GlLayer;

namespace tlp {
class DoubleProperty;
class Graph;
class LayoutProperty;
class SizeProperty;
}


class GlGraphRenderingParameters;

enum RenderingEntitiesFlag {
  RenderingGlEntities = 1,
  RenderingNodes = 2,
  RenderingEdges = 4,
  RenderingNodesEdges = 6,
  RenderingAll = 7
};

struct EntityLODUnit {
  EntityLODUnit(const tlp::BoundingBox &boundingBox):boundingBox(boundingBox),lod(-1) {}
  tlp::BoundingBox boundingBox;
  float lod;
};

struct GlEntityLODUnit : public EntityLODUnit {
  GlEntityLODUnit(GlEntity *glEntity) : EntityLODUnit(glEntity->getBoundingBox()), glEntity(glEntity) {}
  GlEntity *glEntity;

  bool operator==(const GlEntityLODUnit &o) const {
    return glEntity == o.glEntity;
  }
};

struct NodeEntityLODUnit : public EntityLODUnit {
  NodeEntityLODUnit(tlp::node n, const tlp::BoundingBox &boundingBox) : EntityLODUnit(boundingBox), n(n) {}
  tlp::node n;

  bool operator==(const NodeEntityLODUnit &o) const {
    return n == o.n;
  }

};

struct EdgeEntityLODUnit : public EntityLODUnit {
  EdgeEntityLODUnit(tlp::edge e, const tlp::BoundingBox &boundingBox) : EntityLODUnit(boundingBox), e(e) {}
  tlp::edge e;
  float lodSize;

  bool operator==(const EdgeEntityLODUnit &o) const {
    return e == o.e;
  }

};

class GlLODCalculator {

public:

  GlLODCalculator() : _renderingEntitiesFlag(RenderingAll) {}
  virtual ~GlLODCalculator() {}

  virtual void setRenderingEntitiesFlag(RenderingEntitiesFlag flag) {
    _renderingEntitiesFlag=flag;
  }

  virtual void setGraph(tlp::Graph *graph, GlGraphRenderingParameters *renderingParameters) = 0;

  virtual void compute(Camera *camera, const tlp::Vec4i &selectionViewport = tlp::Vec4i(-1,-1,-1,-1)) = 0;

  const std::vector<GlEntityLODUnit> &getGlEntitiesResult(GlLayer *layer) {
    return _glEntitiesLODVector[layer];
  }

  const std::vector<NodeEntityLODUnit> &getNodesResult() {
    return _nodesLODVector;
  }

  const std::vector<EdgeEntityLODUnit> &getEdgesResult() {
    return _edgesLODVector;
  }

  virtual void clear() {
    _glEntitiesLODVector.clear();
    _nodesLODVector.clear();
    _edgesLODVector.clear();
  }

  virtual void setSceneBoundingBox(const tlp::BoundingBox &sceneBoundingBox)=0;

  virtual tlp::BoundingBox getSceneBoundingBox()=0;

  virtual void addGlEntity(GlLayer *layer, GlEntity *glEntity)=0;

  virtual void removeGlEntity(GlLayer *layer, GlEntity *glEntity)=0;

  virtual void removeLayer(GlLayer *layer) {
    _glEntitiesLODVector.erase(layer);
  }

  virtual void addNode(const tlp::node n)=0;

  virtual void addEdge(const tlp::edge e)=0;

protected :

  RenderingEntitiesFlag _renderingEntitiesFlag;

  std::map<GlLayer*, std::vector<GlEntityLODUnit> > _glEntitiesLODVector;

  std::vector<NodeEntityLODUnit> _nodesLODVector;
  std::vector<EdgeEntityLODUnit> _edgesLODVector;

};

#endif // Tulip_GLLODCALCULATOR_H
