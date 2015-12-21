#ifndef SELECTIONMODIFIERINTERACTOR_H
#define SELECTIONMODIFIERINTERACTOR_H

#include "GlSceneInteractor.h"

#include <tulip/BoundingBox.h>

#include <map>
#include <set>

class GlScene;
class GlGraph;
class SelectionInteractor;

namespace tlp {
class Graph;
class BooleanProperty;
class LayoutProperty;
class SizeProperty;
class DoubleProperty;
}

class SelectionModifierInteractor : public GlSceneInteractor {

public:

  SelectionModifierInteractor(GlScene *glScene = NULL);

  void setScene(GlScene *glScene);

  virtual bool mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers);

  virtual bool mouseMoveCallback(int x, int y, const int &modifiers);

  virtual void draw();

  void activate();

  void desactivate();

private:

  bool hasSelection();
  void updateSelectionBoundingBox();
  void translateSelection(tlp::Graph*, int x, int y);

  MouseButton _mouseButton;
  int _lastX, _lastY;

  std::map<tlp::Graph*, tlp::Graph*> _selectionSg;
  std::map<tlp::Graph*, tlp::Graph*> _displayGraph;
  std::map<tlp::Graph*, GlGraph*> _displayGlGraph;
  std::map<tlp::Graph*, tlp::BoundingBox> _selectionBoundingBox;
  SelectionInteractor *_selectionInteractor;

  std::map<tlp::Graph*, tlp::BooleanProperty*> _viewSelection;
  std::map<tlp::Graph*, tlp::LayoutProperty*>_viewLayout;
  std::map<tlp::Graph*, tlp::SizeProperty *> _viewSize;
  std::map<tlp::Graph*, tlp::DoubleProperty *> _viewRotation;

  bool _dragStarted;
  tlp::Graph *_selectedGraph;
  std::set<tlp::node> _previousSelection;

};

#endif // SELECTIONMODIFIERINTERACTOR_H
