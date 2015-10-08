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

#include "GlGraph.h"
#include "Camera.h"
#include "Light.h"
#include "GlRect2D.h"
#include "ZoomAndPanInteractor.h"
#include "RectangleZoomInteractor.h"
#include "SelectionInteractor.h"
#include "SelectionModifierInteractor.h"
#include "NeighborhoodInteractor.h"
#include "LassoSelectionInteractor.h"
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
static std::map<std::string, unsigned int> canvas2dTexture;
static std::map<std::string, bool> canvas2dModified;


static ZoomAndPanInteractor zoomAndPanInteractor;
static RectangleZoomInteractor rectangleZoomInteractor;
static SelectionInteractor selectionInteractor;
static SelectionModifierInteractor selectionModifierInteractor;
static NeighborhoodInteractor neighborhoodInteractor;
static LassoSelectionInteractor lassoSelectionInteractor;

static std::string currentCanvasId;

extern "C" {
void refreshWebGLCanvas(void (*drawFunc)());
void resizeWebGLCanvas(const char *canvasId, int width, int height, bool sizeRelativeToContainer);
void requestFullScreenCanvas(const char *canvasId);
void safeSetTimeout(unsigned int msecs, void (*func)(int value), int value);
void setCurrentCanvas(const char *canvasId);
bool domElementExists(const char *elementId);
bool canXhrOnUrl(const char *url);
void loadImageFromUrl(const char *url, void (*imageLoadedFunc)(const char *, const unsigned char *, unsigned int, unsigned int), void (*errorFunc)(const char *));
void createGlTextureFromCanvas(const char *canvasId);

void drawCanvas2d(const char *canvasId) {
  std::string canvas2dId = canvasId;
  canvas2dId += "-2d";
  if (canvas2dTexture.find(canvasId) == canvas2dTexture.end()) {
    unsigned int texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    canvas2dTexture[canvasId] = texId;
    TextureManager::instance()->addExternalTexture(canvas2dId, texId);
  }

  if (canvas2dModified[canvasId]) {
    glBindTexture(GL_TEXTURE_2D, canvas2dTexture[canvasId]);
    createGlTextureFromCanvas(canvas2dId.c_str());
    glBindTexture(GL_TEXTURE_2D, 0);
    canvas2dModified[canvasId] = false;
  }

  Camera camera2d(false);
  tlp::Vec4i viewport = glScene[canvasId]->getViewport();
  camera2d.setViewport(viewport);
  camera2d.initGl();
  tlp::Vec2f bl(0, 0);
  tlp::Vec2f tr(viewport[2], viewport[3]);
  GlRect2D rect(bl, tr, 0, tlp::Color::White);
  rect.setTexture(canvas2dId);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  rect.draw(camera2d);
  glDisable(GL_BLEND);
}

void EMSCRIPTEN_KEEPALIVE updateGlScene(const char *canvasId) {
  std::string curCanvasBak = currentCanvasId;
  setCurrentCanvas(canvasId);
  glScene[canvasId]->draw();

  if (curCanvasBak == canvasId && currentCanvasInteractor[canvasId]) {
    currentCanvasInteractor[canvasId]->draw();
  }

  //drawCanvas2d(canvasId);

  if (GlShaderProgram::getCurrentActiveShader()) {
    GlShaderProgram::getCurrentActiveShader()->desactivate();
  }
  checkOpenGLError();
  setCurrentCanvas(curCanvasBak.c_str());
}

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
    if (wheelEvent->deltaY > 0) {
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

void glDraw() {
  refreshWebGLCanvas(drawCallback);
}

void timerFunc(unsigned int msecs, void (*func)(int value), int value) {
  safeSetTimeout(msecs, func, value);
}

static int nbTextureToLoad = 0;

static std::vector<std::string> cachedTextureName;

static void onTextureLoaded(const char *texture) {
  TextureManager::instance(currentCanvasId)->addTextureFromFile(texture, true);
  --nbTextureToLoad;
  if (nbTextureToLoad == 0) {
    for (auto it = glScene.begin() ; it != glScene.end() ; ++it) {
      it->second->forceRedraw();
    }
    glDraw();
  }
}

static void onTextureLoadError(const char *texture) {
  std::cout << "Error when loading texture "<< texture << std::endl;
  if (std::string(texture).substr(0, 4) == "http") {
    std::cout << "CORS requests is probably not supported by the server hosting the image" << std::endl;
  }
  --nbTextureToLoad;
  if (nbTextureToLoad == 0) {
    for (auto it = glScene.begin() ; it != glScene.end() ; ++it) {
      it->second->forceRedraw();
    }
    glDraw();
  }
}

static void registerTextureFromData(const char *textureName, const unsigned char *textureData, unsigned int width, unsigned int height) {
  TextureManager::instance(currentCanvasId)->addTextureFromData(textureName, textureData, width, height, true);
  --nbTextureToLoad;
  if (nbTextureToLoad == 0) {
    for (auto it = glScene.begin() ; it != glScene.end() ; ++it) {
      it->second->forceRedraw();
    }
    glDraw();
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
        emscripten_async_wget(cachedTextureName.back().c_str(), cachedTextureName.back().c_str(), onTextureLoaded, onTextureLoadError);
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
      glDraw();
    }
  }

};

static GlDrawObserver glDrawObserver;

extern "C" {

void EMSCRIPTEN_KEEPALIVE activateNavigationInteractor(const char *canvasId) {
  currentCanvasInteractor[canvasId] = &zoomAndPanInteractor;
}

void EMSCRIPTEN_KEEPALIVE activateZoomInteractor(const char *canvasId) {
  currentCanvasInteractor[canvasId] = &rectangleZoomInteractor;
}

void EMSCRIPTEN_KEEPALIVE activateSelectionInteractor(const char *canvasId) {
  currentCanvasInteractor[canvasId] = &selectionInteractor;
}

void EMSCRIPTEN_KEEPALIVE activateSelectionModifierInteractor(const char *canvasId) {
  currentCanvasInteractor[canvasId] = &selectionModifierInteractor;
}

void EMSCRIPTEN_KEEPALIVE activateLassoSelectionInteractor(const char *canvasId) {
  currentCanvasInteractor[canvasId] = &lassoSelectionInteractor;
}

void EMSCRIPTEN_KEEPALIVE activateNeighborhoodInteractor(const char *canvasId) {
  currentCanvasInteractor[canvasId] = &neighborhoodInteractor;
}

void EMSCRIPTEN_KEEPALIVE activateJsInteractor(const char *canvasId) {
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
    canvas2dModified[currentCanvasId] = true;

    resizeWebGLCanvas(canvasIds.back().c_str(), width, height, sizeRelativeToContainer);

    EmscriptenWebGLContextAttributes webGlContextAttributes;
    emscripten_webgl_init_context_attributes(&webGlContextAttributes);
    webGlContextAttributes.stencil = true;
    webGlContextAttributes.alpha = false;

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

    GlLayer *progressLayer = glScene[currentCanvasId]->createLayer("progress", false);
    glProgressBar[currentCanvasId] = new GlProgressBar(tlp::Coord(viewport[2]/2.f, viewport[3]/2.f), 0.8f * viewport[2], 0.1f * viewport[3],
        tlp::Color::Gray, tlp::Color::Black, tlp::Color::Black);

    progressLayer->addGlEntity(glProgressBar[currentCanvasId], "progress bar");
    glProgressBar[currentCanvasId]->setVisible(false);

    activateNavigationInteractor(currentCanvasId.c_str());

    glDraw();

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
    zoomAndPanInteractor.setScene(glScene[canvasId]);
    selectionInteractor.setScene(glScene[canvasId]);
    rectangleZoomInteractor.setScene(glScene[canvasId]);
    selectionModifierInteractor.setScene(glScene[canvasId]);
    neighborhoodInteractor.setScene(glScene[canvasId]);
    lassoSelectionInteractor.setScene(glScene[canvasId]);
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

void EMSCRIPTEN_KEEPALIVE setCanvas2dModified(const char *canvasId) {
  canvas2dModified[canvasId] = true;
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
  //glDraw();
}

void EMSCRIPTEN_KEEPALIVE setCanvasGraph(const char *canvasId, tlp::Graph *g) {
  setCurrentCanvas(canvasId);
  //LabelsRenderer::instance(canvasId)->clearGraphNodesLabelsRenderingData(graph[canvasId]);
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
    glDraw();
  }
}

void EMSCRIPTEN_KEEPALIVE centerScene(const char *canvasId) {
  glScene[canvasId]->centerScene();
  glDraw();
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

void EMSCRIPTEN_KEEPALIVE startGraphViewUpdate(const char *canvasId) {
  glProgressBar[canvasId]->setPercent(-1);
  glGraph[canvasId]->clearObservers();
}

void EMSCRIPTEN_KEEPALIVE endGraphViewUpdate(const char *canvasId) {
  glGraph[canvasId]->prepareEdgesData();
  glGraph[canvasId]->computeGraphBoundingBox();
  glGraph[canvasId]->initObservers();
}

//==============================================================

void EMSCRIPTEN_KEEPALIVE setProgressBarPercent(const char *canvasId, int percent) {
  glProgressBar[canvasId]->setPercent(percent);
}

void EMSCRIPTEN_KEEPALIVE setProgressBarComment(const char *canvasId, const char *comment) {
  glProgressBar[canvasId]->setComment(comment);
}

void EMSCRIPTEN_KEEPALIVE draw() {
  glDraw();
}

void EMSCRIPTEN_KEEPALIVE setGraphRenderingDataReady(const char *canvasId, bool ready) {
  glProgressBar[canvasId]->setVisible(!ready);
  glScene[canvasId]->getMainLayer()->setVisible(ready);
}

void EMSCRIPTEN_KEEPALIVE fullScreen(const char *canvasId) {
  requestFullScreenCanvas(canvasId);
}

}

//==============================================================


