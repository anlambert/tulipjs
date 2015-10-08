#ifndef Tulip_QLQUADTREELODCALCULATOR_H
#define Tulip_QLQUADTREELODCALCULATOR_H

#include <map>
#include <vector>

#include <tulip/Observable.h>

#include "GlCPULODCalculator.h"
#include "Camera.h"

class GlEntity;

namespace tlp {
class PropertyInterface;
class Graph;
}

template <class TYPE> class QuadTreeNode;

class GlQuadTreeLODCalculator : public GlCPULODCalculator {

public:

  GlQuadTreeLODCalculator();
  ~GlQuadTreeLODCalculator();

  void setGraph(tlp::Graph *graph, GlGraphRenderingParameters *renderingParameters);

  void computeFor3DCamera(const tlp::Coord &eye,
                          const tlp::MatrixGL &transformMatrix,
                          const tlp::Vec4i &globalViewport,
                          const tlp::Vec4i &currentViewport);

  void setSceneBoundingBox(const tlp::BoundingBox &sceneBoundingBox);

  bool haveToCompute() const {
    return _haveToCompute;
  }

  void setHaveToCompute(bool haveToCompute) {
    _haveToCompute = haveToCompute;
  }

  void addGlEntity(GlLayer *layer, GlEntity *glEntity);

  void removeGlEntity(GlLayer *layer, GlEntity *glEntity);

  void removeLayer(GlLayer *layer);

  void addNode(const tlp::node n);

  void addEdge(const tlp::edge e);

  void clear();

private :

  void initQuadTree();

  void insertNodeInQuadTree(const tlp::BoundingBox &nodeBoundingBox, const tlp::node n);
  void insertEdgeInQuadTree(tlp::BoundingBox edgeBoundingBox, const tlp::edge e);

  std::map<GlLayer*, QuadTreeNode<GlEntity*> *> _glEntitiesQuadTree;
  QuadTreeNode<tlp::node> *_nodesQuadTree;
  QuadTreeNode<tlp::edge> *_edgesQuadTree;

  bool _haveToCompute;

};

#endif // Tulip_QTQUADTREELODCALCULATOR_H
