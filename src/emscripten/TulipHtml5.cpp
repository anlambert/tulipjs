#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <ctime>

#include <tulip/Graph.h>
#include <tulip/GraphAbstract.h>
#include <tulip/TlpTools.h>
#include <tulip/PluginLibraryLoader.h>
#include <tulip/PropertyTypes.h>
#include <tulip/LayoutProperty.h>
#include <tulip/GraphProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/ColorProperty.h>
#include <tulip/BooleanProperty.h>
#include <tulip/IntegerProperty.h>
#include <tulip/DoubleProperty.h>
#include <tulip/StringProperty.h>
#include <tulip/PluginLister.h>
#include <tulip/ExportModule.h>
#include <tulip/SimplePluginProgress.h>
#include <tulip/Color.h>
#include <tulip/PropertyTypes.h>
#include <tulip/ColorScale.h>

#include "NanoVGManager.h"
#include "GlGraph.h"
#include "Camera.h"
#include "Light.h"
#include "GlRect2D.h"
#include "Utils.h"
#include "TextureManager.h"
#include "LabelsRenderer.h"
#include "GlScene.h"
#include "GlLayer.h"
#include "GlLODCalculator.h"
#include "ConcaveHullBuilder.h"
#include "GlConcavePolygon.h"
#include "ShaderManager.h"
#include "GlShaderProgram.h"
#include "glyphs/GlyphsRenderer.h"
#include "GlProgressBar.h"
#include "GlGraphRenderingParameters.h"
#include "Interactors.h"


#include <sys/stat.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <dlfcn.h>

static std::vector<std::string> canvasIds;
static std::map<std::string, EMSCRIPTEN_WEBGL_CONTEXT_HANDLE> webGlContextHandle;
static std::map<std::string, GlScene *> glScene;
static std::map<std::string, GlGraph *> glGraph;
static std::map<std::string, GlProgressBar *> glProgressBar;
static std::map<std::string, tlp::Graph *> graph;
static std::map<tlp::Graph *, std::string> graphToCanvas;
static std::map<std::string, GlSceneInteractor*> currentCanvasInteractor;

static std::map<std::string, GlLayer *> hullsLayer;
static std::map<std::string, std::map<unsigned int, GlConcavePolygon *> > subgraphsHulls;


static std::string currentCanvasId;

extern "C" {
void refreshWebGLCanvas(void (*drawFunc)());
void resizeWebGLCanvas(const char *canvasId, int width, int height, bool sizeRelativeToContainer);
void requestFullScreenCanvas(const char *canvasId);
void safeSetTimeout(unsigned int msecs, void (*func)(void *value), void *value);
void setCurrentCanvas(const char *canvasId);
bool domElementExists(const char *elementId);
bool canXhrOnUrl(const char *url);
void loadImageFromUrl(const char *url, void (*imageLoadedFunc)(const char *, const unsigned char *, unsigned int, unsigned int), void (*errorFunc)(unsigned int, void *, int));
bool isChrome();

void EMSCRIPTEN_KEEPALIVE updateGlScene(const char *canvasId) {
  std::string curCanvasBak = currentCanvasId;
  setCurrentCanvas(canvasId);

  glScene[canvasId]->draw();

  if (curCanvasBak == canvasId && currentCanvasInteractor[canvasId]) {
    currentCanvasInteractor[canvasId]->draw();
  }

  if (GlShaderProgram::getCurrentActiveShader()) {
    GlShaderProgram::getCurrentActiveShader()->desactivate();
  }

  checkOpenGLError();
  setCurrentCanvas(curCanvasBak.c_str());
}

static void drawCallback(void) {
  for (size_t i = 0 ; i < canvasIds.size() ; ++i) {
    if (domElementExists(canvasIds[i].c_str())) {
      updateGlScene(canvasIds[i].c_str());
    } else {
      std::cout << canvasIds[i] << " does not exist anymore" << std::endl;
    }
  }
}

void EMSCRIPTEN_KEEPALIVE draw(bool refreshDisplay = true) {
  if (refreshDisplay) {
    refreshWebGLCanvas(drawCallback);
  } else {
    drawCallback();
  }
}

}

static int getModifiers(const EmscriptenMouseEvent &mouseEvent) {
  int modifiers = 0;
  if (mouseEvent.ctrlKey) {
    modifiers |= ACTIVE_CTRL;
  }
  if (mouseEvent.altKey) {
    modifiers |= ACTIVE_ALT;
  }
  if (mouseEvent.shiftKey) {
    modifiers |= ACTIVE_SHIFT;
  }
  return modifiers;
}

static int getModifiers(const EmscriptenKeyboardEvent &keyEvent) {
  int modifiers = 0;
  if (keyEvent.ctrlKey) {
    modifiers |= ACTIVE_CTRL;
  }
  if (keyEvent.altKey) {
    modifiers |= ACTIVE_ALT;
  }
  if (keyEvent.shiftKey) {
    modifiers |= ACTIVE_SHIFT;
  }
  return modifiers;
}

static EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
  std::string canvasId = canvasIds[reinterpret_cast<int>(userData)];
  if (currentCanvasInteractor[canvasId]) {
    MouseButton button;
    if (mouseEvent->button == 0) {
      button = LEFT_BUTTON;
    } else if (mouseEvent->button == 1) {
      button = MIDDLE_BUTTON;
    } else  {
      button = RIGHT_BUTTON;
    }
    if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
      return currentCanvasInteractor[canvasId]->mouseCallback(button, DOWN, mouseEvent->targetX, mouseEvent->targetY, getModifiers(*mouseEvent));
    } else if (eventType == EMSCRIPTEN_EVENT_MOUSEUP) {
      return currentCanvasInteractor[canvasId]->mouseCallback(button, UP, mouseEvent->targetX, mouseEvent->targetY, getModifiers(*mouseEvent));
    } else if (eventType == EMSCRIPTEN_EVENT_MOUSEMOVE) {
      return currentCanvasInteractor[canvasId]->mouseMoveCallback(mouseEvent->targetX, mouseEvent->targetY, getModifiers(*mouseEvent));
    } else if (eventType == EMSCRIPTEN_EVENT_MOUSEENTER) {
      setCurrentCanvas(canvasId.c_str());
    } else if (eventType == EMSCRIPTEN_EVENT_MOUSELEAVE) {
      currentCanvasInteractor[canvasId]->mouseCallback(button, UP, 0, 0, getModifiers(*mouseEvent));
      setCurrentCanvas("");
    }
  }
  return false;
}

static EM_BOOL wheelCallback(int /* eventType */, const EmscriptenWheelEvent *wheelEvent, void *userData) {
  std::string canvasId = canvasIds[reinterpret_cast<int>(userData)];
  if (currentCanvasInteractor[canvasId]) {
    double delta = wheelEvent->deltaY;
    if (wheelEvent->mouse.shiftKey && isChrome()) {
      delta = wheelEvent->deltaX;
    }
    if (delta > 0) {
      return currentCanvasInteractor[canvasId]->mouseCallback(WHEEL, DOWN, wheelEvent->mouse.targetX, wheelEvent->mouse.targetY, getModifiers(wheelEvent->mouse));
    } else {
      return currentCanvasInteractor[canvasId]->mouseCallback(WHEEL, UP, wheelEvent->mouse.targetX, wheelEvent->mouse.targetY, getModifiers(wheelEvent->mouse));
    }
  }
  return false;
}

static EM_BOOL keyCallback(int /* eventType */, const EmscriptenKeyboardEvent *keyEvent, void *userData) {

  std::string canvasId = canvasIds[reinterpret_cast<int>(userData)];
  if (currentCanvasInteractor[canvasId]) {
    return currentCanvasInteractor[canvasId]->keyboardCallback(keyEvent->key, getModifiers(*keyEvent));
  }
  return false;
}

static std::ostringstream oss;
static std::string staticString;

void timerFunc(unsigned int msecs, void (*func)(void *value), void *value) {
  safeSetTimeout(msecs, func, value);
}

static int nbTextureToLoad = 0;

static std::vector<std::string> cachedTextureName;

static void onTextureLoaded(unsigned int, void*, const char *texture) {
  TextureManager::instance(currentCanvasId)->addTextureFromFile(texture, true);
  --nbTextureToLoad;
  if (nbTextureToLoad == 0) {
    for (auto it = glScene.begin() ; it != glScene.end() ; ++it) {
      it->second->forceRedraw();
    }
    draw();
  }
}

static void onTextureLoadError(unsigned int, void *texture, int) {
  std::cout << "Error when loading texture "<< reinterpret_cast<const char*>(texture) << std::endl;
  if (std::string(reinterpret_cast<const char*>(texture)).substr(0, 4) == "http") {
    std::cout << "CORS requests is probably not supported by the server hosting the image" << std::endl;
  }
  --nbTextureToLoad;
  if (nbTextureToLoad == 0) {
    for (auto it = glScene.begin() ; it != glScene.end() ; ++it) {
      it->second->forceRedraw();
    }
    draw();
  }
}

static void registerTextureFromData(const char *textureName, const unsigned char *textureData, unsigned int width, unsigned int height) {
  TextureManager::instance(currentCanvasId)->addTextureFromData(textureName, textureData, width, height, true);
  --nbTextureToLoad;
  if (nbTextureToLoad == 0) {
    for (auto it = glScene.begin() ; it != glScene.end() ; ++it) {
      it->second->forceRedraw();
    }
    draw();
  }
}

static void loadTexture(const std::string &texture) {
  if (!texture.empty()) {
    if (std::find(cachedTextureName.begin(), cachedTextureName.end(), texture) == cachedTextureName.end()) {
      cachedTextureName.push_back(texture);
      ++nbTextureToLoad;
      if (texture.substr(0, 4) == "http") {
        loadImageFromUrl(cachedTextureName.back().c_str(), registerTextureFromData, onTextureLoadError);
      } else {
        if (!canXhrOnUrl(cachedTextureName.back().c_str())) {
          return;
        }
        std::vector<std::string> paths;
        tokenize(texture, paths, "/");
        if (paths.size() > 1) {
          std::string curPath;
          for (size_t i = 0 ; i < paths.size() - 1 ; ++i) {
            curPath += "/" + paths[i];
            mkdir(curPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
          }
        }
        emscripten_async_wget2(cachedTextureName.back().c_str(), cachedTextureName.back().c_str(), "GET", "",
                               reinterpret_cast<void*>(const_cast<char*>(cachedTextureName.back().c_str())), onTextureLoaded, onTextureLoadError, NULL);
      }
    }
  }
}

class GlDrawObserver : public tlp::Observable {

public:

  void treatEvent(const tlp::Event &event) {
    const GlSceneEvent *glSceneEvent = dynamic_cast<const GlSceneEvent*>(&event);
    const GlLayerEvent *glLayerEvent = dynamic_cast<const GlLayerEvent*>(&event);
    const tlp::PropertyEvent *propertyEvent = dynamic_cast<const tlp::PropertyEvent*>(&event);


    if (glSceneEvent) {
      if (glSceneEvent->getType() == GlSceneEvent::LAYER_ADDED_IN_SCENE) {
        glSceneEvent->getGlLayer()->addListener(this);
      } else if (glSceneEvent->getType() == GlSceneEvent::LAYER_REMOVED_FROM_SCENE) {
        glSceneEvent->getGlLayer()->removeListener(this);
      } else if(glSceneEvent->getType() == GlSceneEvent::DRAW_REQUEST) {
        draw();
      }
    }

    if (glLayerEvent) {
      if (glLayerEvent->getType() == GlLayerEvent::ENTITY_ADDED_IN_LAYER) {
        glLayerEvent->getGlEntity()->addObserver(this);
      } else if (glLayerEvent->getType() == GlLayerEvent::ENTITY_REMOVED_FROM_LAYER) {
        glLayerEvent->getGlEntity()->removeObserver(this);
      }
    }

    if (propertyEvent) {
      tlp::PropertyInterface *prop = propertyEvent->getProperty();
      if (propertyEvent->getType() == tlp::PropertyEvent::TLP_AFTER_SET_NODE_VALUE &&
          prop->getName() == "viewTexture") {
        loadTexture(prop->getNodeStringValue(propertyEvent->getNode()));
      }
      if (propertyEvent->getType() == tlp::PropertyEvent::TLP_AFTER_SET_EDGE_VALUE &&
          prop->getName() == "viewTexture") {
        loadTexture(prop->getEdgeStringValue(propertyEvent->getEdge()));
      }
      if (propertyEvent->getType() == tlp::PropertyEvent::TLP_AFTER_SET_ALL_NODE_VALUE &&
          prop->getName() == "viewTexture") {
        loadTexture(prop->getNodeDefaultStringValue());
      }
      if (propertyEvent->getType() == tlp::PropertyEvent::TLP_AFTER_SET_ALL_EDGE_VALUE &&
          prop->getName() == "viewTexture") {
        loadTexture(prop->getEdgeDefaultStringValue());
      }
    }
  }

  void treatEvents(const std::vector<tlp::Event> &events) {
    bool drawNeeded = false;
    for (size_t i = 0 ; i < events.size() ; ++i) {
      GlEntity *glEntity = dynamic_cast<GlEntity*>(events[i].sender());
      drawNeeded = drawNeeded || (glEntity != NULL);
    }
    if (drawNeeded) {
      draw();
    }
  }

};

static GlDrawObserver glDrawObserver;

extern "C" {

void EMSCRIPTEN_KEEPALIVE activateInteractor(const char *canvasId, const char *interactorName) {
  if (interactorsMap.find(interactorName) == interactorsMap.end()) {
    std::cerr << "Error : no interactor named \"" << interactorName << "\"" << std::endl;
  } else {
    if (currentCanvasInteractor[canvasId]) {
      currentCanvasInteractor[canvasId]->desactivate();
    }
    currentCanvasInteractor[canvasId] = interactorsMap[interactorName];
    currentCanvasInteractor[canvasId]->setScene(glScene[canvasId]);
    currentCanvasInteractor[canvasId]->activate();
  }
}

void EMSCRIPTEN_KEEPALIVE desactivateInteractor(const char *canvasId) {
  if (currentCanvasInteractor[canvasId]) {
    currentCanvasInteractor[canvasId]->desactivate();
  }
  currentCanvasInteractor[canvasId] = NULL;
}

void cleanManagedCanvasIfNeeded() {
  std::vector<std::string> removedCanvas;
  for (size_t i = 0 ; i < canvasIds.size() ; ++i) {
    if (!domElementExists(canvasIds[i].c_str())) {
      removedCanvas.push_back(canvasIds[i]);
    }
  }
  for (size_t i = 0 ; i < removedCanvas.size() ; ++i) {
    canvasIds.erase(std::remove(canvasIds.begin(), canvasIds.end(), removedCanvas[i]), canvasIds.end());
    emscripten_webgl_destroy_context(webGlContextHandle[removedCanvas[i]]);
    delete glScene[removedCanvas[i]];
    glScene.erase(removedCanvas[i]);
    graph.erase(removedCanvas[i]);
    glProgressBar.erase(removedCanvas[i]);
    glGraph.erase(removedCanvas[i]);
    graphToCanvas.erase(graph[removedCanvas[i]]);
    graph.erase(removedCanvas[i]);
    currentCanvasInteractor.erase(removedCanvas[i]);
  }
}

void EMSCRIPTEN_KEEPALIVE initCanvas(const char *canvasId, int width, int height, bool sizeRelativeToContainer) {

  cleanManagedCanvasIfNeeded();

  currentCanvasId = canvasId;

  if (glScene.find(currentCanvasId) == glScene.end() || !glScene[currentCanvasId]) {

    std::ostringstream oss;
    oss << "document.getElementById('" << currentCanvasId << "').setAttribute('tabindex', '" << canvasIds.size() << "');";
    emscripten_run_script(oss.str().c_str());

    canvasIds.push_back(currentCanvasId);

    resizeWebGLCanvas(canvasIds.back().c_str(), width, height, sizeRelativeToContainer);

    EmscriptenWebGLContextAttributes webGlContextAttributes;
    emscripten_webgl_init_context_attributes(&webGlContextAttributes);
    webGlContextAttributes.stencil = true;
    webGlContextAttributes.alpha = false;
    webGlContextAttributes.preserveDrawingBuffer = true;

    webGlContextHandle[canvasId] = emscripten_webgl_create_context(canvasId, &webGlContextAttributes);

    setCurrentCanvas(canvasId);

    emscripten_set_mousedown_callback(canvasIds.back().c_str(), reinterpret_cast<void*>(canvasIds.size()-1), false, mouseCallback);
    emscripten_set_mouseup_callback(canvasIds.back().c_str(), reinterpret_cast<void*>(canvasIds.size()-1), false, mouseCallback);
    emscripten_set_mousemove_callback(canvasIds.back().c_str(), reinterpret_cast<void*>(canvasIds.size()-1), false, mouseCallback);
    emscripten_set_mouseenter_callback(canvasIds.back().c_str(), reinterpret_cast<void*>(canvasIds.size()-1), false, mouseCallback);
    emscripten_set_mouseleave_callback(canvasIds.back().c_str(), reinterpret_cast<void*>(canvasIds.size()-1), false, mouseCallback);
    emscripten_set_wheel_callback(canvasIds.back().c_str(), reinterpret_cast<void*>(canvasIds.size()-1), false, wheelCallback);
    emscripten_set_keypress_callback(canvasIds.back().c_str(), reinterpret_cast<void*>(canvasIds.size()-1), false, keyCallback);

    tlp::Vec4i viewport(0, 0, width, height);

    glScene[currentCanvasId] = new GlScene();
    glScene[currentCanvasId]->addListener(&glDrawObserver);
    glScene[currentCanvasId]->getMainLayer()->addListener(&glDrawObserver);


    glScene[currentCanvasId]->getMainLayer()->getLight()->setDirectionnalLight(true);
    glScene[currentCanvasId]->setViewport(viewport);

    glGraph[currentCanvasId] = new GlGraph();
    glScene[currentCanvasId]->getMainLayer()->addGlEntity(glGraph[currentCanvasId], "graph");

    hullsLayer[currentCanvasId] = new GlLayer("hulls", glScene[currentCanvasId]->getMainLayer()->getCamera(), glScene[currentCanvasId]->getMainLayer()->getLight());

    GlLayer *progressLayer = glScene[currentCanvasId]->createLayer("progress", false);
    glProgressBar[currentCanvasId] = new GlProgressBar(tlp::Coord(viewport[2]/2.f, viewport[3]/2.f), 0.8f * viewport[2], 0.1f * viewport[3],
        tlp::Color::Gray, tlp::Color::Black, tlp::Color::Black);

    progressLayer->addGlEntity(glProgressBar[currentCanvasId], "progress bar");
    glProgressBar[currentCanvasId]->setVisible(false);

    activateInteractor(currentCanvasId.c_str(), "ZoomAndPan");

    draw();

  }

}

void EMSCRIPTEN_KEEPALIVE setCurrentCanvas(const char *canvasId) {
  currentCanvasId = canvasId;
  if (std::string(canvasId) != "") {
    emscripten_webgl_make_context_current(webGlContextHandle[canvasId]);
    ShaderManager::setCurrentCanvasId(canvasId);
    TextureManager::setCurrentCanvasId(canvasId);
    LabelsRenderer::setCurrentCanvasId(canvasId);
    GlyphsRenderer::setCurrentCanvasId(canvasId);
    NanoVGManager::setCurrentCanvasId(canvasId);
    if (currentCanvasInteractor[canvasId]) {
      currentCanvasInteractor[canvasId]->setScene(glScene[canvasId]);
    }
  }
  std::ostringstream oss;
  for (size_t i = 0 ; i < canvasIds.size() ; ++i) {
    if (canvasIds[i] != currentCanvasId) {
      oss.str("");
      oss << "if (document.getElementById('" << canvasIds[i] << "')) document.getElementById('" << canvasIds[i] << "').blur();";
      emscripten_run_script(oss.str().c_str());
    }
  }
  if (std::string(canvasId) != "") {
    oss.str("");
    oss << "if (document.getElementById('" << canvasId << "')) document.getElementById('" << canvasId << "').focus();";
    emscripten_run_script(oss.str().c_str());
  }

}

const char * EMSCRIPTEN_KEEPALIVE getCurrentCanvas() {
  return currentCanvasId.c_str();
}

void EMSCRIPTEN_KEEPALIVE resizeCanvas(const char *canvasId, int width, int height, bool sizeRelativeToContainer) {
  resizeWebGLCanvas(canvasId, width, height, sizeRelativeToContainer);
  tlp::Vec4i viewport(0, 0, width, height);
  glScene[canvasId]->setViewport(viewport);
  glProgressBar[canvasId]->setCenter(tlp::Coord(viewport[2]/2.f, viewport[3]/2.f));
  glProgressBar[canvasId]->setWidth(0.8f * viewport[2]);
  glProgressBar[canvasId]->setHeight(0.1f * viewport[3]);
}

bool EMSCRIPTEN_KEEPALIVE subGraphHasHull(const char *canvasId, tlp::Graph *sg) {
  return subgraphsHulls[canvasId].find(sg->getId()) != subgraphsHulls[canvasId].end();
}

void EMSCRIPTEN_KEEPALIVE addSubGraphHull(const char *canvasId, tlp::Graph *sg) {
  tlp::Color hullColor = genRandomColor(100);
  if (subgraphsHulls[canvasId].find(sg->getId()) != subgraphsHulls[canvasId].end()) {
    hullsLayer[canvasId]->deleteGlEntity(subgraphsHulls[canvasId][sg->getId()]);
    delete subgraphsHulls[canvasId][sg->getId()];
  }
  std::ostringstream oss;
  oss << "hull_" << sg->getId();
  subgraphsHulls[canvasId][sg->getId()] = new GlConcavePolygon(computeGraphHullVertices(sg, 0, false), hullColor);
  hullsLayer[canvasId]->addGlEntity(subgraphsHulls[canvasId][sg->getId()], oss.str());
}

void EMSCRIPTEN_KEEPALIVE setSubGraphsHullsVisible(const char *canvasId, bool visible, bool onTop = true) {
  glScene[canvasId]->removeLayer(hullsLayer[canvasId]);
  if (visible) {
    if (onTop) {
      glScene[canvasId]->addExistingLayerAfter(hullsLayer[canvasId], "Main");
    } else {
      glScene[canvasId]->addExistingLayerBefore(hullsLayer[canvasId], "Main");
    }
  }
}

void EMSCRIPTEN_KEEPALIVE clearSubGraphsHulls(const char *canvasId) {
  hullsLayer[canvasId]->clear(true);
  subgraphsHulls[canvasId].clear();
}

void EMSCRIPTEN_KEEPALIVE setCanvasGraph(const char *canvasId, tlp::Graph *g) {
  setCurrentCanvas(canvasId);

  clearSubGraphsHulls(canvasId);

  tlp::StringProperty *viewTexture = g->getProperty<tlp::StringProperty>("viewTexture");

  tlp::node n;
  nbTextureToLoad = 0;
  forEach(n, g->getNodes()) {
    loadTexture(viewTexture->getNodeValue(n));
  }
  viewTexture->addListener(&glDrawObserver);
  graph[canvasId] = g;
  graphToCanvas[g] = canvasId;
  glGraph[canvasId]->setGraph(g);

  glScene[canvasId]->centerScene();
  glProgressBar[currentCanvasId]->setVisible(false);
  if (nbTextureToLoad == 0) {
    draw();
  }
}

void EMSCRIPTEN_KEEPALIVE centerScene(const char *canvasId) {
  glScene[canvasId]->centerScene();
  draw();
}

static std::vector<SelectedEntity> selectedNodes;
static std::vector<SelectedEntity> selectedEdges;

unsigned int EMSCRIPTEN_KEEPALIVE selectNodes(const char *canvasId, int x, int y, int w, int h) {
  int width = w;
  int height = h;
  if (w == 0) {
    x -= 1;
    y -= 1;
    width = 3;
    height = 3;
  }
  selectedNodes.clear();
  glScene[canvasId]->selectEntities(RenderingNodes, x, y, width, height, selectedNodes);
  return selectedNodes.size();
}

void EMSCRIPTEN_KEEPALIVE getSelectedNodes(unsigned int *nodesIds) {
  for (size_t i = 0 ; i < selectedNodes.size() ; ++i) {
    *nodesIds++ = selectedNodes[i].getNode().id;
  }
}

unsigned int EMSCRIPTEN_KEEPALIVE selectEdges(const char *canvasId, int x, int y, int w, int h) {
  int width = w;
  int height = h;
  if (w == 0) {
    x -= 1;
    y -= 1;
    width = 3;
    height = 3;
  }
  selectedEdges.clear();
  glScene[canvasId]->selectEntities(RenderingEdges, x, y, width, height, selectedEdges);
  return selectedEdges.size();
}

void EMSCRIPTEN_KEEPALIVE getSelectedEdges(unsigned int *edgesIds) {
  for (size_t i = 0 ; i < selectedEdges.size() ; ++i) {
    *edgesIds++ = selectedEdges[i].getEdge().id;
  }
}

void EMSCRIPTEN_KEEPALIVE startGraphViewData(const char *canvasId) {
  LabelsRenderer::instance(canvasId)->clearGraphNodesLabelsRenderingData(graph[canvasId]);
  glGraph[canvasId]->setGraph(graph[canvasId]);
  glGraph[canvasId]->clearObservers();
}

void EMSCRIPTEN_KEEPALIVE endGraphViewData(const char *canvasId) {
  glGraph[canvasId]->initObservers();
  glGraph[canvasId]->prepareNodesLabelsData();
  glGraph[canvasId]->prepareEdgesData();
  glGraph[canvasId]->computeGraphBoundingBox();
}

void EMSCRIPTEN_KEEPALIVE startGraphViewUpdate(const char *canvasId, bool clearGraph = false) {
  glProgressBar[canvasId]->setPercent(-1);
  glGraph[canvasId]->clearObservers();
  if (clearGraph) {
    graph[canvasId]->clear();
  }
}

void EMSCRIPTEN_KEEPALIVE endGraphViewUpdate(const char *canvasId) {
  glGraph[canvasId]->prepareEdgesData();
  glGraph[canvasId]->computeGraphBoundingBox();
  glGraph[canvasId]->initObservers();
}

//==============================================================

GlGraphRenderingParameters* EMSCRIPTEN_KEEPALIVE getViewRenderingParameters(const char *canvasId) {
  return &(glGraph[canvasId]->getRenderingParameters());
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setDisplayNodes(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setDisplayNodes(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_displayNodes(GlGraphRenderingParameters *glgrp) {
  return glgrp->displayNodes();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setBillboardedNodes(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setBillboardedNodes(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_billboardedNodes(GlGraphRenderingParameters *glgrp) {
  return glgrp->billboardedNodes();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setDisplayNodesLabels(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setDisplayNodesLabels(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_displayNodesLabels(GlGraphRenderingParameters *glgrp) {
  return glgrp->displayNodesLabels();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setLabelsScaled(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setLabelsScaled(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_labelsScaled(GlGraphRenderingParameters *glgrp) {
  return glgrp->labelsScaled();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setBillboardedLabels(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setBillboardedLabels(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_billboardedLabels(GlGraphRenderingParameters *glgrp) {
  return glgrp->billboardedLabels();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setDisplayEdges(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setDisplayEdges(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_displayEdges(GlGraphRenderingParameters *glgrp) {
  return glgrp->displayEdges();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setInterpolateEdgesColors(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setInterpolateEdgesColors(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_interpolateEdgesColors(GlGraphRenderingParameters *glgrp) {
  return glgrp->interpolateEdgesColors();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setInterpolateEdgesSizes(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setInterpolateEdgesSizes(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_interpolateEdgesSizes(GlGraphRenderingParameters *glgrp) {
  return glgrp->interpolateEdgesSizes();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setDisplayEdgesExtremities(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setDisplayEdgesExtremities(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_displayEdgesExtremities(GlGraphRenderingParameters *glgrp) {
  return glgrp->displayEdgesExtremities();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setEdges3D(GlGraphRenderingParameters *glgrp, bool state) {
  glgrp->setEdges3D(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_edges3D(GlGraphRenderingParameters *glgrp) {
  return glgrp->edges3D();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setMinSizeOfLabels(GlGraphRenderingParameters *glgrp, float minLabelsSize) {
  glgrp->setMinSizeOfLabels(minLabelsSize);
}

float EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_minSizeOfLabels(GlGraphRenderingParameters *glgrp) {
  return glgrp->minSizeOfLabels();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setMaxSizeOfLabels(GlGraphRenderingParameters *glgrp, float maxLabelsSize) {
  glgrp->setMaxSizeOfLabels(maxLabelsSize);
}

float EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_maxSizeOfLabels(GlGraphRenderingParameters *glgrp) {
  return glgrp->maxSizeOfLabels();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setElementsOrdered(GlGraphRenderingParameters *glgrp, const bool state) {
  glgrp->setElementsOrdered(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_elementsOrdered(GlGraphRenderingParameters *glgrp) {
  return glgrp->elementsOrdered();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setElementOrderedDescending(GlGraphRenderingParameters *glgrp, const bool state) {
  glgrp->setElementOrderedDescending(state);
}

bool EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_elementsOrderedDescending(GlGraphRenderingParameters *glgrp) {
  return glgrp->elementsOrderedDescending();
}

void EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_setElementsOrderingProperty(GlGraphRenderingParameters *glgrp, tlp::NumericProperty* property) {
  glgrp->setElementsOrderingProperty(property);
}

tlp::NumericProperty* EMSCRIPTEN_KEEPALIVE GlGraphRenderingParameters_elementsOrderingProperty(GlGraphRenderingParameters *glgrp) {
  return glgrp->elementsOrderingProperty();
}

//==============================================================

void EMSCRIPTEN_KEEPALIVE setProgressBarPercent(const char *canvasId, int percent) {
  glProgressBar[canvasId]->setPercent(percent);
}

void EMSCRIPTEN_KEEPALIVE setProgressBarComment(const char *canvasId, const char *comment) {
  glProgressBar[canvasId]->setComment(comment);
}

void EMSCRIPTEN_KEEPALIVE setGraphRenderingDataReady(const char *canvasId, bool ready) {
  glProgressBar[canvasId]->setVisible(!ready);
  glScene[canvasId]->getMainLayer()->setVisible(ready);
  hullsLayer[canvasId]->setVisible(ready);
}

void EMSCRIPTEN_KEEPALIVE fullScreen(const char *canvasId) {
  requestFullScreenCanvas(canvasId);
}

}

//==============================================================
