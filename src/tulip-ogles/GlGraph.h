#ifndef GRAPH_RENDERER_H
#define GRAPH_RENDERER_H

#include <tulip/Graph.h>
#include <tulip/BoundingBox.h>
#include <tulip/Coord.h>
#include <tulip/Color.h>
#include <tulip/Size.h>
#include <tulip/TulipViewSettings.h>

#include <vector>
#include <map>
#include <set>

#include "GlEntity.h"
#include "GlGraphRenderingParameters.h"

class Glyph;
class LabelsRenderer;
class GlShaderProgram;
class GlBuffer;
class GlLODCalculator;

namespace tlp {
class BooleanProperty;
class ColorProperty;
class DoubleProperty;
class IntegerProperty;
class LayoutProperty;
class SizeProperty;
class StringProperty;
}

class GlGraph : public GlEntity {
  
public:
  
  GlGraph(tlp::Graph *graph = NULL, GlLODCalculator *lodCalculator = NULL);

  ~GlGraph();
  
  void setGraph(tlp::Graph *graph);

  tlp::Graph *graph() const {
    return _graph;
  }

  void draw(const Camera &camera, const Light &light, bool pickingMode=false);

  bool pickNodesAndEdges(const Camera &camera,
                         const int x, const int y,
                         const int width, const int height,
                         std::set<tlp::node> &selectedNodes,
                         std::set<tlp::edge> &selectedEdges);


  bool pickNodeOrEdge(const Camera &camera,
                      const int x, const int y,
                      tlp::node &pickedNode,
                      tlp::edge &pickedEdge);

  void treatEvents(const std::vector<tlp::Event> &);

  void treatEvent(const tlp::Event &message);

  void computeGraphBoundingBox();

  void prepareNodeLabelData(const tlp::node n);
  void prepareNodesLabelsData();

  void startEdgesData();
  void addEdgeData(const tlp::edge e);
  void endEdgesData();
  void prepareEdgesData();

  void initObservers();
  void clearObservers();

  GlLODCalculator *getLODCalculator() const {
    return _lodCalculator;
  }

  GlGraphRenderingParameters& getRenderingParameters() {
    return _renderingParameters;
  }

  void setRenderingParameters(const GlGraphRenderingParameters &renderingParameters);

private :
  
  void initGraphProperties();

  void getEdgeExtremityData(tlp::edge e, bool srcGlyph, tlp::Coord &position, tlp::Size &size, tlp::Vec4f &rotationAxisAndAngle);

  void renderNodes(const Camera &camera, const Light &light);
  void renderNodesGlow(const std::vector<tlp::node> &nodes, const Camera &camera, const Light &light);
  void renderMetaNodes(const std::vector<tlp::node> &metaNodes, const Camera &camera, const Light &light);
  void renderPointsNodesAndEdges(const Camera &camera, const std::vector<tlp::node> &pointsNodes, const std::vector<tlp::edge> &pointsEdges);

  void renderEdges(const Camera &camera, const std::vector<tlp::edge> &edges, bool lineMode, bool billboard=false);

  void renderEdgeExtremities(const Camera &camera, const Light &light);

  void setGraphElementsPickingMode(const bool pickingMode) {
    _graphElementsPickingMode = pickingMode;
  }

  void setSelectionViewport(const tlp::Vec4i &selectionViewport) {
    _selectionViewport = selectionViewport;
  }

  void prepareEdgeData(tlp::edge e);

  tlp::Size getEdgeSize(tlp::edge e);

  tlp::BoundingBox labelBoundingBoxForNode(tlp::node n);

  tlp::Graph *_graph;
  tlp::ColorProperty *_viewColor;
  tlp::LayoutProperty *_viewLayout;
  tlp::SizeProperty *_viewSize;
  tlp::DoubleProperty *_viewRotation;
  tlp::DoubleProperty *_viewBorderWidth;
  tlp::ColorProperty *_viewBorderColor;
  tlp::StringProperty *_viewLabel;
  tlp::IntegerProperty *_viewShape;
  tlp::StringProperty *_viewTexture;
  tlp::BooleanProperty *_viewSelection;
  tlp::IntegerProperty *_viewSrcAnchorShape;
  tlp::IntegerProperty *_viewTgtAnchorShape;
  tlp::SizeProperty *_viewSrcAnchorSize;
  tlp::SizeProperty *_viewTgtAnchorSize;
  tlp::StringProperty *_viewFontAwesomeIcon;
  tlp::BooleanProperty *_viewGlow;
  tlp::ColorProperty *_viewLabelColor;

  GlShaderProgram *_flatShader;

  tlp::BoundingBox _sceneBoundingBox;
  
  std::map<tlp::edge, std::vector<tlp::Vec3f> > _edgePoints;
  std::map<tlp::edge, std::pair<tlp::Vec3f, tlp::Vec3f> > _edgeAnchors;
  std::map<tlp::edge, std::vector<unsigned int> > _edgeLineVerticesIndices;

  GlBuffer *_edgeRenderingDataBuffer;
  GlBuffer *_edgeIndicesBuffer;
  GlBuffer *_curveEdgeRenderingDataBuffer;
  GlBuffer *_curveEdgeIndicesBuffer;
  GlBuffer *_edgeLineRenderingDataBuffer;
  GlBuffer *_edgeLineRenderingIndicesBuffer;

  std::map<int, GlShaderProgram *> _edgesShaders;

  GlShaderProgram *getEdgeShader(int edgeShape);

  GlBuffer *_pointsDataBuffer;

  std::map<int, std::vector<tlp::node> > _nodesGlyphs;

  LabelsRenderer *_labelsRenderer;

  bool _canUseUIntIndices;

  bool _graphElementsPickingMode;

  std::set<tlp::edge> _edgesToUpdate;
  std::set<tlp::node> _nodesToUpdate;

  std::set<tlp::edge> _edgesToDiscard;
  std::set<tlp::node> _nodesToDiscard;

  GlLODCalculator *_lodCalculator;
  tlp::Vec4i _selectionViewport;

  std::map<tlp::edge, std::vector<tlp::Vec3f> > _srcEdgeExtremitiesData;
  std::map<tlp::edge, std::vector<tlp::Vec3f> > _tgtEdgeExtremitiesData;
  std::map<int, std::vector<tlp::edge> > _srcEdgeExtremityGlyphs, _tgtEdgeExtremityGlyphs;

  size_t _maxEdgePoints;

  tlp::BoundingBox _lastGraphBoundingBox;

  GlGraphRenderingParameters _renderingParameters;

  bool _updateQuadTree;

};

#endif
