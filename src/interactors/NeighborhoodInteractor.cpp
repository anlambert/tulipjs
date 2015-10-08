#include "NeighborhoodInteractor.h"
#include "ZoomAndPanInteractor.h"
#include "GlScene.h"
#include "GlLayer.h"
#include "GlGraph.h"
#include "ConcaveHullBuilder.h"
#include "GlRect2D.h"
#include "GlCPULODCalculator.h"

NeighborhoodInteractor::NeighborhoodInteractor(GlScene *scene) : _glGraph(NULL), _neighborhoodSg(NULL) {
  _glScene = scene;
  _znpInteractor = new ZoomAndPanInteractor(scene);
}

void NeighborhoodInteractor::setScene(GlScene *glScene) {
  GlSceneInteractor::setScene(glScene);
  _znpInteractor->setScene(glScene);
}

bool NeighborhoodInteractor::mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers) {
  if (!_glScene) return false;
  tlp::Vec4i viewport = _glScene->getViewport();
  if (x < viewport[0] || x > viewport[2] || y < viewport[1] || y > viewport[3]) return false;
  if (button == LEFT_BUTTON && state == UP) {
    SelectedEntity selectedEntity;
    _glScene->selectEntity(RenderingNodes, x, viewport[3] - y, selectedEntity);
    if (selectedEntity.getEntityType() == SelectedEntity::NODE_SELECTED) {
      destroyNeighborhoodSubgraph();
      _glGraph = selectedEntity.getGlGraph();
      _centralNode = selectedEntity.getNode();
      buildNeighborhoodSubgraph();
      glDraw();
    } else {
      if (_neighborhoodSg) {
        destroyNeighborhoodSubgraph();
        glDraw();
      }
    }
    return _znpInteractor->mouseCallback(button, state, x, y, modifiers);
  } else {
    return _znpInteractor->mouseCallback(button, state, x, y, modifiers);
  }
}

void NeighborhoodInteractor::buildNeighborhoodSubgraph() {
  if (_glGraph) {
    _neighborhoodSg = _glGraph->graph()->addSubGraph();
    _neighborhoodSg->addNode(_centralNode);
    tlp::node neigh;
    forEach(neigh, _glGraph->graph()->getInOutNodes(_centralNode)) {
      _neighborhoodSg->addNode(neigh);
      _neighborhoodSg->addEdge(_glGraph->graph()->existEdge(_centralNode, neigh, false));
    }
    _glNeighborhoodGraph = new GlGraph(_neighborhoodSg, new GlCPULODCalculator);
#ifdef __EMSCRIPTEN__
    _glNeighborhoodGraph->prepareNodesLabelsData();
#endif
  }
}

void NeighborhoodInteractor::destroyNeighborhoodSubgraph() {
  if (_neighborhoodSg) {
    delete _glNeighborhoodGraph;
    _glGraph->graph()->delSubGraph(_neighborhoodSg);
    _glGraph = NULL;
    _neighborhoodSg = NULL;
    _centralNode = tlp::node();
  }
}

bool NeighborhoodInteractor::mouseMoveCallback(int x, int y, const int &modifiers) {
  return _znpInteractor->mouseMoveCallback(x, y, modifiers);
}

void NeighborhoodInteractor::draw() {
  if (!_glScene) return;
  if (_neighborhoodSg) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Camera *camera = _glScene->getMainLayer()->getCamera();
    Camera camera2d(false);
    tlp::Vec4i viewport = camera->getViewport();
    camera2d.setViewport(viewport);
    camera2d.initGl();
    tlp::Vec2f bl(viewport[0], viewport[1]);
    tlp::Vec2f tr(viewport[2], viewport[3]);
    GlRect2D rect(bl, tr, 0, tlp::Color(255,255,255,200));
    rect.draw(camera2d);
    _glScene->getMainLayer()->getCamera()->initGl();
    _glNeighborhoodGraph->setRenderingParameters(_glGraph->getRenderingParameters());
    _glNeighborhoodGraph->draw(*camera, *(_glScene->getMainLayer()->getLight()));
  }
}
