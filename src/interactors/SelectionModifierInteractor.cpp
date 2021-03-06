#include "GlGraph.h"
#include "SelectionModifierInteractor.h"
#include "Camera.h"
#include "GlScene.h"
#include "SelectionInteractor.h"
#include "GlRect2D.h"
#include "GlLayer.h"
#include "GlCPULODCalculator.h"

#include <tulip/Graph.h>
#include <tulip/BooleanProperty.h>
#include <tulip/LayoutProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/DoubleProperty.h>
#include <tulip/DrawingTools.h>

using namespace tlp;

SelectionModifierInteractor::SelectionModifierInteractor(GlScene *glScene) :
  _lastX(-1), _lastY(-1),
  _selectionInteractor(NULL), _dragStarted(false),
  _selectedGraph(NULL) {
  _glScene = glScene;
  _selectionInteractor = new SelectionInteractor(glScene);
  _selectionInteractor->setSelectOnlyEdgesConnectedToSelectedNodes(true);
}

void SelectionModifierInteractor::setScene(GlScene *glScene) {
  GlSceneInteractor::setScene(glScene);
  _selectionInteractor->setScene(glScene);
}

bool SelectionModifierInteractor::mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers) {
  if (!_glScene) return false;
  _mouseButton = button;
  Camera *camera = _glScene->getMainLayer()->getCamera();
  tlp::Vec4i viewport = camera->getViewport();
  Coord scenePos = camera->screenTo3DWorld(Coord(viewport[2] - x, y));
  if (std::abs(scenePos[2]) < 1e-3) {
    scenePos[2] = 0;
  }
  bool selection = hasSelection();
  std::map<tlp::Graph*, tlp::BoundingBox>::iterator it = _selectionBoundingBox.begin();
  for (; it != _selectionBoundingBox.end() ; ++it) {
    bool pointerInSelectionBB = _selectionBoundingBox[it->first].isValid() && _selectionBoundingBox[it->first].contains(scenePos);
    if (pointerInSelectionBB) {
      _selectedGraph = it->first;
      break;
    }
  }
  if (button == WHEEL || (button == LEFT_BUTTON && (!selection || !_selectedGraph))) {
    _dragStarted = false;
    _selectionInteractor->mouseCallback(button, state, x, y, modifiers);
    hasSelection();
    _glScene->requestDraw();
    return true;
  } else if (button == LEFT_BUTTON) {
    if (state == DOWN && _selectedGraph) {
      _dragStarted = true;
      _lastX = x;
      _lastY = y;
      return true;
    } else if (state == UP && _dragStarted) {
      _dragStarted = false;
      _lastX = -1;
      _lastY = -1;
      _selectedGraph = NULL;
      return true;
    }
  }
  return false;
}

bool SelectionModifierInteractor::mouseMoveCallback(int x, int y, const int &modifiers) {
  if (!_glScene) return false;
  bool selection = hasSelection();
  if (!_dragStarted && (_mouseButton == LEFT_BUTTON && (!selection || !_selectedGraph))) {
    return _selectionInteractor->mouseMoveCallback(x, y, modifiers);
  } else if ((_dragStarted && _selectedGraph) || (_mouseButton == LEFT_BUTTON && selection && _selectedGraph)) {
    translateSelection(_selectedGraph, _lastX - x, y - _lastY);
    _lastX = x;
    _lastY = y;
    _glScene->requestDraw();
    return true;
  }
  return false;
}

void SelectionModifierInteractor::draw() {
  if (!_glScene) return;
  Camera *camera = _glScene->getMainLayer()->getCamera();
  Light *light = _glScene->getMainLayer()->getLight();
  _selectionInteractor->draw();
  updateSelectionBoundingBox();
  camera->initGl();
  glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  std::map<tlp::Graph*, GlGraph *>::iterator itG = _displayGlGraph.begin();
  for(; itG != _displayGlGraph.end() ; ++itG) {
    itG->second->draw(*camera, *light);
  }
  std::map<tlp::Graph*, tlp::BoundingBox>::iterator it = _selectionBoundingBox.begin();
  for (; it != _selectionBoundingBox.end() ;++it) {
    BoundingBox selectionBoundingBox = it->second;
    if (selectionBoundingBox.isValid()) {
      GlRect2D rect(tlp::Vec2f(selectionBoundingBox[0][0], selectionBoundingBox[0][1]), tlp::Vec2f(selectionBoundingBox[1][0], selectionBoundingBox[1][1]), selectionBoundingBox[1][2], tlp::Color(0,0,0,100), tlp::Color::Black);
      rect.draw(*camera);
    }
  }
  glDisable(GL_BLEND);
}

bool SelectionModifierInteractor::hasSelection() {
  tlp::Observable::holdObservers();
  bool ret = false;
  std::set<GlEntity*> entities = _glScene->getEntities();
  for (std::set<GlEntity*>::iterator it = entities.begin() ; it != entities.end() ; ++it) {
    GlGraph *glGraph = dynamic_cast<GlGraph*>(*it);
    if (glGraph && glGraph->graph()) {
      Graph *graph = glGraph->graph()->getSuperGraph();
      if (_selectionSg.find(graph) == _selectionSg.end()) {
        _selectionSg[graph] = NULL;
      }
      _viewSelection[graph] = graph->getProperty<BooleanProperty>("viewSelection");
      _viewLayout[graph] = graph->getProperty<LayoutProperty>("viewLayout");
      _viewSize[graph] = graph->getProperty<SizeProperty>("viewSize");
      _viewRotation[graph] = graph->getProperty<DoubleProperty>("viewRotation");
      Iterator<node> *selectedNodes = _viewSelection[graph]->getNodesEqualTo(true);
      ret = ret || selectedNodes->hasNext();
      if (selectedNodes->hasNext()) {
        node n;
        edge e;
        std::set<node> setNodes;
        forEach(n, selectedNodes) {
          setNodes.insert(n);
        }

        if (setNodes == _previousSelection) {
          continue;
        }

        _previousSelection = setNodes;

        if (_selectionSg[graph] != NULL) {
          graph->delSubGraph(_selectionSg[graph]);
        }
        _selectionSg[graph] = graph->inducedSubGraph(setNodes);
        if (_displayGraph.find(graph) == _displayGraph.end()) {
          _displayGraph[graph] = graph->addSubGraph();
        }

        stableForEach(n, _displayGraph[graph]->getNodes()) {
          if (!_selectionSg[graph]->isElement(n)) {
            _displayGraph[graph]->delNode(n);
            glGraph->graph()->addNode(n);
            forEach(e, graph->getInOutEdges(n)) {
              glGraph->graph()->addEdge(e);
            }
          }
        }
        forEach(n, _selectionSg[graph]->getNodes()) {
          if (glGraph->graph()->isElement(n))
            glGraph->graph()->delNode(n);
          _displayGraph[graph]->addNode(n);

          forEach(e, graph->getInOutEdges(n)) {
            _displayGraph[graph]->addEdge(e);
          }
        }
        if (_displayGlGraph.find(graph) == _displayGlGraph.end()) {
          _displayGlGraph[graph] = new GlGraph(_displayGraph[graph], new GlCPULODCalculator);
        }

      } else {
        delete selectedNodes;
        if (_selectionSg[graph] != NULL) {
          tlp::node n;
          forEach(n, _displayGraph[graph]->getNodes()) {
            glGraph->graph()->addNode(n);
          }
          tlp::edge e;
          forEach(e, _displayGraph[graph]->getEdges()) {
            glGraph->graph()->addEdge(e);
          }
          _displayGraph[graph]->clear();
          graph->delSubGraph(_selectionSg[graph]);
        }
        _selectionSg[graph] = NULL;
        _previousSelection.clear();
      }
    }
  }
  tlp::Observable::unholdObservers();
  return ret;
}

void SelectionModifierInteractor::updateSelectionBoundingBox() {
  hasSelection();
  std::map<tlp::Graph*, tlp::Graph*>::iterator it = _selectionSg.begin();
  for (; it != _selectionSg.end() ; ++it) {
    if (it->second != NULL) {
      _selectionBoundingBox[it->first] = computeBoundingBox(it->second, _viewLayout[it->first], _viewSize[it->first], _viewRotation[it->first]);
      Coord center = _selectionBoundingBox[it->first].center();
      float hWidth = _selectionBoundingBox[it->first].width()/2;
      float hHeight = _selectionBoundingBox[it->first].height()/2;
      float hDepth = _selectionBoundingBox[it->first].depth()/2;
      _selectionBoundingBox[it->first][0] = center - 1.1f * Coord(hWidth, hHeight, hDepth);
      _selectionBoundingBox[it->first][1] = center + 1.1f * Coord(hWidth, hHeight, hDepth);
    } else {
      _selectionBoundingBox[it->first] = BoundingBox();
    }
  }
}

void SelectionModifierInteractor::translateSelection(Graph *graph, int x, int y) {
  tlp::Observable::holdObservers();
  tlp::Coord v1(0, 0, 0);
  tlp::Coord v2(x, y, 0);
  Camera *camera = _glScene->getMainLayer()->getCamera();
  v1 = camera->screenTo3DWorld(v1);
  v2 = camera->screenTo3DWorld(v2);
  tlp::Coord move = v2 - v1;
  _viewLayout[graph]->translate(move, _selectionSg[graph]);
  tlp::Observable::unholdObservers();
}

void SelectionModifierInteractor::activate() {
  std::set<GlEntity*> entities = _glScene->getEntities();
  for (std::set<GlEntity*>::iterator it = entities.begin() ; it != entities.end() ; ++it) {
    GlGraph *glGraph = dynamic_cast<GlGraph*>(*it);
    if (glGraph && glGraph->graph()) {
      tlp::Graph *clone = glGraph->graph()->addCloneSubGraph("tmpClone");
      glGraph->setGraph(clone);
    }
  }
}

void SelectionModifierInteractor::desactivate() {
  std::map<tlp::Graph*, GlGraph *>::iterator itG = _displayGlGraph.begin();
  for(; itG != _displayGlGraph.end() ; ++itG) {
    delete itG->second;
  }
  _displayGlGraph.clear();
  std::map<tlp::Graph*, tlp::Graph *>::iterator it = _displayGraph.begin();
  for(; it != _displayGraph.end() ; ++it) {
    it->first->delSubGraph(it->second);
  }
  _displayGraph.clear();
  it = _selectionSg.begin();
  for(; it != _selectionSg.end() ; ++it) {
    if (it->second) {
      it->first->delSubGraph(it->second);
    }
  }
  _selectionSg.clear();
  std::set<GlEntity*> entities = _glScene->getEntities();
  for (std::set<GlEntity*>::iterator it = entities.begin() ; it != entities.end() ; ++it) {
    GlGraph *glGraph = dynamic_cast<GlGraph*>(*it);
    if (glGraph && glGraph->graph()) {
      tlp::Graph *original = glGraph->graph()->getSuperGraph();
      glGraph->setGraph(original);
      original->delSubGraph(original->getSubGraph("tmpClone"));
    }
  }
  _previousSelection.clear();
}
