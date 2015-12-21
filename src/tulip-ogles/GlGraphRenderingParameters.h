#ifndef GLGRAPHRENDERINGPARAMETERS_H
#define GLGRAPHRENDERINGPARAMETERS_H

#include <tulip/Color.h>
#include <tulip/DataSet.h>
#include <tulip/Observable.h>

namespace tlp {
class NumericProperty;
class BooleanProperty;
}

class GlGraph;

class GlGraphRenderingParameters : public tlp::Observable {

public:

  GlGraphRenderingParameters();

  void setGlGraph(GlGraph *glGraph);
  GlGraph *glGraph() const;

  tlp::DataSet getParameters() const;
  void setParameters(const tlp::DataSet &);

  void setDisplayEdgesExtremities(const bool state);
  bool displayEdgesExtremities()const;

  void setDisplayNodesLabels(const bool state);
  bool displayNodesLabels() const;

  void setDisplayEdgesLabels(const bool state);
  bool displayEdgesLabels() const;

  void setInterpolateEdgesColors(const bool state);
  bool interpolateEdgesColors() const;

  void setInterpolateEdgesSizes(const bool state);
  bool interpolateEdgesSizes() const;

  void setDisplayMetaNodesLabels(const bool state);
  bool displayMetaNodesLabels() const;

  void setDisplayEdges(const bool state);
  bool displayEdges() const;

  void setDisplayNodes(const bool state);
  bool displayNodes() const;

  void setDisplayMetaNodes(const bool state);
  bool displayMetaNodes() const;

  void setNodesStencil(const int stencil);
  int nodesStencil() const;

  void setMetaNodesStencil(const int stencil);
  int metaNodesStencil() const;

  void setEdgesStencil(const int stencil);
  int edgesStencil() const;

  void setNodesLabelsStencil(const int stencil);
  int nodesLabelsStencil() const;

  void setMetaNodesLabelsStencil(const int stencil);
  int metaNodesLabelsStencil() const;

  void setEdgesLabelsStencil(const int stencil);
  int edgesLabelsStencil() const;

  void setElementsOrdered(const bool state);
  bool elementsOrdered() const;

  void setElementsOrderingProperty(tlp::NumericProperty* property);
  tlp::NumericProperty* elementsOrderingProperty() const;

  void setElementsZOrdered(const bool state);
  bool elementsZOrdered() const;

  void setEdges3D(const bool state);
  bool edges3D() const;

  void setMaxEdgesSizesToNodesSizes(const bool b);
  bool maxEdgesSizesToNodesSizes() const;

  void setSelectionColor(const tlp::Color &color);
  tlp::Color selectionColor() const;

  void setLabelsScaled(bool state);
  bool labelsScaled() const;

  void setLabelsDensity(int density);
  int labelsDensity() const;

  void setMinSizeOfLabels(float size);
  float minSizeOfLabels() const;

  void setMaxSizeOfLabels(float size);
  float maxSizeOfLabels() const;

  void setBillboardedNodes(bool billboardedNodes);
  bool billboardedNodes() const;

  void setBillboardedLabels(bool billboarded);
  bool billboardedLabels() const;

  void setDisplayFilteringProperty(tlp::BooleanProperty* filteringProperty);
  tlp::BooleanProperty* displayFilteringProperty() const;

  void setBypassLodSystem(bool bypassLodSystem);
  bool bypassLodSystem() const;

private:

  GlGraph *_glGraph;
  bool _displayEdgesExtremities;
  bool _displayNodesLabels;
  bool _displayEdgesLabels;
  bool _displayMetaNodesLabels;
  bool _elementsOrdered;
  bool _interpolateEdgesColors;
  bool _edges3D;
  bool _interpolateEdgesSizes;
  bool _displayEdges;
  bool _displayNodes;
  bool _displayMetaNodes;
  bool _elementsZOrdered;
  int _nodesStencil;
  int _metaNodesStencil;
  int _edgesStencil;
  int _nodesLabelsStencil;
  int _metaNodesLabelsStencil;
  int _edgesLabelsStencil;
  bool _labelsScaled;
  float _labelsMinSize;
  float _labelsMaxSize;
  int _labelsDensity;
  bool _billboardedNodes;
  bool _billboardedLabels;
  bool _maxEdgesSizesToNodesSizes;
  tlp::Color _selectionColor;
  tlp::BooleanProperty *_displayFilteringProperty;
  tlp::NumericProperty* _elementsOrderingProperty;
  bool _bypassLodSystem;
};

class GlGraphRenderingParametersEvent : public tlp::Event {

public:

  enum GlGraphRenderingParametersEventType {RENDERING_PARAMETERS_MODIFIED = 0,
                                            DISPLAY_EDGES_EXTREMITIES_TOGGLED,
                                            INTERPOLATE_EDGES_COLORS_TOGGLED,
                                            INTERPOLATE_EDGES_SIZES_TOGGLED};

  GlGraphRenderingParametersEvent(GlGraphRenderingParameters *renderingParameters,
                                  GlGraphRenderingParametersEventType eventType) :
    tlp::Event(*renderingParameters, tlp::Event::TLP_MODIFICATION), _eventType(eventType) {}

  GlGraphRenderingParametersEventType getType() const {
    return _eventType;
  }

  GlGraphRenderingParameters *getRenderingParameters() const {
    return static_cast<GlGraphRenderingParameters*>(sender());
  }

private:

  GlGraphRenderingParametersEventType _eventType;

};

#endif
