#ifndef LABELSRENDERER_H
#define LABELSRENDERER_H

#include <tulip/BoundingBox.h>
#include <tulip/Graph.h>
#include <tulip/StringProperty.h>
#include <tulip/LayoutProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/ColorProperty.h>
#include <tulip/BooleanProperty.h>

#include <vector>
#include <map>

#include "Camera.h"

class LabelsRenderer {

public :

  static LabelsRenderer *instance();

  static LabelsRenderer *instance(const std::string &canvasId);

  static void setCurrentCanvasId(const std::string &canvasId) {
    _currentCanvasId = canvasId;
  }

  ~LabelsRenderer();

  void setFontFile(const std::string &fontFile) {
    _fontFile = fontFile;
    _fontHandle = -1;
  }

  void initFont();

  bool fontInit() const {
    return _fontHandle != -1;
  }

  std::string fontFile() const {
    return _fontFile;
  }

  void addOrUpdateNodeLabel(tlp::Graph *graph, tlp::node n);

  void removeNodeLabel(tlp::Graph *graph, tlp::node n);

  void setLabelsScaled(const bool labelsScaled) {
    _labelsScaled = labelsScaled;
  }

  bool labelsScaled() const {
    return _labelsScaled;
  }

  void setMinMaxSizes(float minSize, float maxSize) {
    _minSize = minSize;
    _maxSize = maxSize;
  }

  void setOcclusionTest(const bool occlusionTest) {
    _occlusionTest = occlusionTest;
  }

  void renderGraphNodesLabels(tlp::Graph *graph, const Camera &camera, const tlp::Color &selectionColor, bool billboarded = false);

  void renderOneLabel(const Camera &camera, const std::string &text, const tlp::BoundingBox &renderingBox,
                      const tlp::Color &labelColor = tlp::Color::Black);

  void clearGraphNodesLabelsRenderingData(tlp::Graph *graph);

  void setGraphNodesLabelsToRender(tlp::Graph *graph, const std::vector<tlp::node> &labelsToRender) {
    _labelsToRender[graph] = labelsToRender;
  }

private :

  static std::map<std::string, LabelsRenderer *> _instances;
  static std::string _currentCanvasId;


  LabelsRenderer();

  float getTextAspectRatio(const std::string &text);

  tlp::BoundingBox getLabelRenderingBoxScaled(const tlp::BoundingBox &renderingBox, float textAspectRatio);

  std::string _fontFile;
  int _fontHandle;

  std::map<tlp::Graph *, std::vector<tlp::node> > _labelsToRender;
  std::map<tlp::Graph *, std::map<tlp::node, float> > _nodeLabelAspectRatio;
  std::map<tlp::Graph *, std::map<tlp::node, unsigned int> > _nodeLabelNbLines;

  bool _labelsScaled;
  float _minSize, _maxSize;
  bool _occlusionTest;

};

#endif // LABELSRENDERER_H
