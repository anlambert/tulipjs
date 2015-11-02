#include "GlGraphRenderingParameters.h"

using namespace tlp;

GlGraphRenderingParameters::GlGraphRenderingParameters() :
  _glGraph(NULL),
  _displayEdgesExtremities(true),
  _displayNodesLabels(true),
  _displayEdgesLabels(false),
  _displayMetaNodesLabels(false),
  _elementsOrdered(false),
  _interpolateEdgesColors(false),
  _edges3D(false),
  _interpolateEdgesSizes(true),
  _displayEdges(true),
  _displayNodes(true),
  _displayMetaNodes(true),
  _elementsZOrdered(false),
  _nodesStencil(0x01),
  _metaNodesStencil(0x01),
  _edgesStencil(0x02),
  _nodesLabelsStencil(0x01),
  _metaNodesLabelsStencil(0x01),
  _edgesLabelsStencil(0x02),
  _labelsScaled(false),
  _labelsMinSize(12),
  _labelsMaxSize(72),
  _labelsDensity(0),
  _billboardedNodes(false),
  _billboardedLabels(false),
  _maxEdgesSizesToNodesSizes(true),
  _selectionColor(Color(255,0,255)),
  _displayFilteringProperty(NULL),
  _elementsOrderingProperty(NULL),
  _bypassLodSystem(false)
{
}
//====================================================
void GlGraphRenderingParameters::setGlGraph(GlGraph *glGraph) {
  _glGraph = glGraph;
}
GlGraph *GlGraphRenderingParameters::glGraph() const {
  return _glGraph;
}
//====================================================
//TODO
DataSet GlGraphRenderingParameters::getParameters() const {
  DataSet data;
  return data;
}
void GlGraphRenderingParameters::setParameters(const DataSet & /* data */) {}
//====================================================
bool GlGraphRenderingParameters::displayMetaNodesLabels()const {
  return _displayMetaNodesLabels;
}
void GlGraphRenderingParameters::setDisplayMetaNodesLabels(bool b) {
  _displayMetaNodesLabels=b;
}
//====================================================
bool GlGraphRenderingParameters::displayEdges() const {
  return _displayEdges;
}
void GlGraphRenderingParameters::setDisplayEdges(const bool b) {
  if (b != _displayEdges) {
    _displayEdges=b;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
//====================================================
bool GlGraphRenderingParameters::displayNodes() const {
  return _displayNodes;
}
void GlGraphRenderingParameters::setDisplayNodes(const bool b) {
  if (b != _displayNodes) {
    _displayNodes=b;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
//====================================================
bool GlGraphRenderingParameters::displayMetaNodes() const {
  return _displayMetaNodes;
}
void GlGraphRenderingParameters::setDisplayMetaNodes(const bool b) {
  _displayMetaNodes=b;
}
//====================================================
bool GlGraphRenderingParameters::displayEdgesExtremities() const {
  return _displayEdgesExtremities;
}
void GlGraphRenderingParameters::setDisplayEdgesExtremities(const bool b) {
  if (b != _displayEdgesExtremities) {
    _displayEdgesExtremities=b;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::DISPLAY_EDGES_EXTREMITIES_TOGGLED));
  }
}
//====================================================
bool GlGraphRenderingParameters::elementsOrdered() const {
  return _elementsOrdered;
}
void GlGraphRenderingParameters::setElementsOrdered(const bool b) {
  _elementsOrdered = b;
}
//====================================================
bool GlGraphRenderingParameters::elementsZOrdered() const {
  return _elementsZOrdered;
}
void GlGraphRenderingParameters::setElementsZOrdered(const bool b) {
  _elementsZOrdered = b;
}
//====================================================
bool GlGraphRenderingParameters::edges3D() const {
  return _edges3D;
}
void GlGraphRenderingParameters::setEdges3D(const bool b) {
  if (b != _edges3D) {
    _edges3D=b;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
//====================================================
bool GlGraphRenderingParameters::displayNodesLabels()const {
  return _displayNodesLabels;
}
void GlGraphRenderingParameters::setDisplayNodesLabels(const bool b) {
  if (_displayNodesLabels != b) {
    _displayNodesLabels=b;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
//====================================================
bool GlGraphRenderingParameters::displayEdgesLabels()const {
  return _displayEdgesLabels;
}
void GlGraphRenderingParameters::setDisplayEdgesLabels(const bool b) {
  _displayEdgesLabels=b;
}
//====================================================
void GlGraphRenderingParameters::setNodesStencil(const int stencil) {
  _nodesStencil=stencil;
}
int GlGraphRenderingParameters::nodesStencil() const {
  return _nodesStencil;
}
//====================================================
void GlGraphRenderingParameters::setMetaNodesStencil(const int stencil) {
  _metaNodesStencil=stencil;
}
int GlGraphRenderingParameters::metaNodesStencil() const {
  return _metaNodesStencil;
}
//====================================================
void GlGraphRenderingParameters::setEdgesStencil(const int stencil) {
  _edgesStencil=stencil;
}
int GlGraphRenderingParameters::edgesStencil() const {
  return _edgesStencil;
}
//====================================================
void GlGraphRenderingParameters::setNodesLabelsStencil(const int stencil) {
  _nodesLabelsStencil=stencil;
}
int GlGraphRenderingParameters::nodesLabelsStencil() const {
  return _nodesLabelsStencil;
}
//====================================================
void GlGraphRenderingParameters::setMetaNodesLabelsStencil(const int stencil) {
  _metaNodesLabelsStencil=stencil;
}
int GlGraphRenderingParameters::metaNodesLabelsStencil() const {
  return _metaNodesLabelsStencil;
}
//====================================================
void GlGraphRenderingParameters::setEdgesLabelsStencil(const int stencil) {
  _edgesLabelsStencil=stencil;
}
int GlGraphRenderingParameters::edgesLabelsStencil() const {
  return _edgesLabelsStencil;
}
//====================================================
bool GlGraphRenderingParameters::interpolateEdgesColors()const {
  return _interpolateEdgesColors;
}
void GlGraphRenderingParameters::setInterpolateEdgesColors(const bool b) {
  if (b != _interpolateEdgesColors) {
    _interpolateEdgesColors=b;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
//====================================================
bool GlGraphRenderingParameters::interpolateEdgesSizes() const {
  return _interpolateEdgesSizes;
}
void GlGraphRenderingParameters::setInterpolateEdgesSizes(const bool b) {
  if (b != _interpolateEdgesSizes) {
    _interpolateEdgesSizes=b;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
//====================================================
bool GlGraphRenderingParameters::maxEdgesSizesToNodesSizes() const {
  return _maxEdgesSizesToNodesSizes;
}
void GlGraphRenderingParameters::setMaxEdgesSizesToNodesSizes(const bool b) {
  _maxEdgesSizesToNodesSizes=b;
}
//====================================================
void GlGraphRenderingParameters::setSelectionColor(const Color &color) {
  _selectionColor=color;
}
Color GlGraphRenderingParameters::selectionColor() const {
  return _selectionColor;
}
//====================================================
bool GlGraphRenderingParameters::labelsScaled() const {
  return _labelsScaled;
}
void GlGraphRenderingParameters::setLabelsScaled(bool state) {
  if (_labelsScaled != state) {
    _labelsScaled=state;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
//====================================================
int GlGraphRenderingParameters::labelsDensity() const {
  return _labelsDensity;
}
void GlGraphRenderingParameters::setLabelsDensity(int density) {
  _labelsDensity=density;
}
//====================================================
float GlGraphRenderingParameters::minSizeOfLabels() const {
  return _labelsMinSize;
}
void GlGraphRenderingParameters::setMinSizeOfLabels(float size) {
  _labelsMinSize=size;
}
//====================================================
float GlGraphRenderingParameters::maxSizeOfLabels() const {
  return _labelsMaxSize;
}
void GlGraphRenderingParameters::setMaxSizeOfLabels(float size) {
  _labelsMaxSize=size;
}
//====================================================
void GlGraphRenderingParameters::setElementsOrderingProperty(tlp::NumericProperty* property) {
  _elementsOrderingProperty = property;
}
tlp::NumericProperty* GlGraphRenderingParameters::elementsOrderingProperty() const {
  return _elementsOrderingProperty;
}
//====================================================
void GlGraphRenderingParameters::setBillboardedNodes(bool billboardedNodes) {
  if (billboardedNodes != _billboardedNodes) {
    _billboardedNodes = billboardedNodes;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
bool GlGraphRenderingParameters::billboardedNodes() const {
  return _billboardedNodes;
}
//====================================================
void GlGraphRenderingParameters::setBillboardedLabels(bool billboarded) {
  if (billboarded != _billboardedLabels) {
    _billboardedLabels=billboarded;
    sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
bool GlGraphRenderingParameters::billboardedLabels() const {
  return _billboardedLabels;
}
//====================================================
void GlGraphRenderingParameters::setDisplayFilteringProperty(tlp::BooleanProperty *filteringProperty) {
  _displayFilteringProperty=filteringProperty;
}
tlp::BooleanProperty* GlGraphRenderingParameters::displayFilteringProperty() const {
  return _displayFilteringProperty;
}
//====================================================
void GlGraphRenderingParameters::setBypassLodSystem(bool bypassLodSystem) {
  if (_bypassLodSystem != bypassLodSystem) {
     _bypassLodSystem = bypassLodSystem;
     sendEvent(GlGraphRenderingParametersEvent(this, GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED));
  }
}
bool GlGraphRenderingParameters::bypassLodSystem() const {
    return _bypassLodSystem;
}
