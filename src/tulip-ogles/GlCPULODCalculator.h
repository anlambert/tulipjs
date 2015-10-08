#ifndef Tulip_GLCPULODCALCULATOR_H
#define Tulip_GLCPULODCALCULATOR_H

#include "GlLODCalculator.h"
#include "Camera.h"

#include <tulip/Coord.h>
#include <tulip/Size.h>
#include <tulip/Matrix.h>
#include <tulip/Observable.h>

#include <map>

namespace tlp {
class DoubleProperty;
class IntegerProperty;
class LayoutProperty;
class SizeProperty;
}

class GlGraphRenderingParameters;

class GlCPULODCalculator : public GlLODCalculator {

public:

  GlCPULODCalculator();
  ~GlCPULODCalculator();

  void setGraph(tlp::Graph *graph, GlGraphRenderingParameters *renderingParameters);

  void compute(Camera *camera, const tlp::Vec4i &selectionViewport = tlp::Vec4i(-1,-1,-1,-1));

  void setSceneBoundingBox(const tlp::BoundingBox &sceneBoundingBox) {
    _sceneBoundingBox = sceneBoundingBox;
  }

  tlp::BoundingBox getSceneBoundingBox() {
    return _sceneBoundingBox;
  }

  void addGlEntity(GlLayer *layer, GlEntity *glEntity);

  void removeGlEntity(GlLayer *layer, GlEntity *glEntity);

  void addNode(const tlp::node n);

  void addEdge(const tlp::edge e);

protected:

  tlp::BoundingBox getNodeBoundingBox(tlp::node n);
  tlp::BoundingBox getEdgeBoundingBox(tlp::edge e);

  virtual void computeFor3DCamera(const tlp::Coord &eye, const tlp::MatrixGL &transformMatrix, const tlp::Vec4i &globalViewport, const tlp::Vec4i &currentViewport);

  virtual void computeFor2DCamera(const tlp::Vec4i &globalViewport, const tlp::Vec4i &currentViewport);

  tlp::BoundingBox _sceneBoundingBox;

  tlp::Graph *_graph;
  tlp::DoubleProperty *_viewRotation;
  tlp::LayoutProperty *_viewLayout;
  tlp::SizeProperty *_viewSize;
  tlp::IntegerProperty *_viewShape;

  Camera *camera;

  std::map<tlp::node, tlp::BoundingBox> _nodesBBCache;
  std::map<tlp::edge, tlp::BoundingBox> _edgesBBCache;
  std::map<tlp::Size, float> _edgeSizeLodCache;

  GlGraphRenderingParameters *_renderingParameters;

};

#endif // Tulip_GLCPULODCALCULATOR_H

