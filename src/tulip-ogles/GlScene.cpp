#include <GL/glew.h>

#include <algorithm>
#include <cstdlib>
#include <climits>
#include <cstdio>
#include <sstream>

#include "GlScene.h"
#include "GlLayer.h"
#include "GlQuadTreeLODCalculator.h"
#include "GlBoundingBoxSceneVisitor.h"
#include "GlLODSceneVisitor.h"
#include "GlRect2D.h"
#include "GlShaderProgram.h"
#include "GlFrameBufferObject.h"
#include "Utils.h"
#include "GlGraph.h"
#include "TextureManager.h"

using namespace std;
using namespace tlp;

GlScene::GlScene() : _backgroundColor(255, 255, 255, 255), _clearBufferAtDraw(true), _sceneNeedRedraw(true),
  _backBufferTexture(0), _backBufferBackup(NULL), _pickingMode(false), _backupBackBuffer(true) {
  _lodCalculator = new GlQuadTreeLODCalculator();
  _lodCalculator->setRenderingEntitiesFlag(RenderingGlEntities);
  _mainLayer = new GlLayer("Main");
  addExistingLayer(_mainLayer);
  ostringstream oss;
  oss << "backBufferTexture" << reinterpret_cast<unsigned long>(this);
  _backBufferTextureName = oss.str();

}

GlScene::~GlScene() {

  for(vector<pair<string, GlLayer *> >::iterator it=_layersList.begin(); it!=_layersList.end(); ++it) {
    delete it->second;
  }

  delete _lodCalculator;
  delete [] _backBufferBackup;
}

void GlScene::setViewport(const Vec4i &viewport) {
  if (_viewport != viewport) {
    _viewport=viewport;
    _sceneNeedRedraw = true;
    if (_backupBackBuffer) {
      delete [] _backBufferBackup;
      _backBufferBackup = new unsigned char[viewport[2]*viewport[3]*4];
    }
  }
}

void GlScene::setViewport(int x, int y, int width, int height) {
  setViewport(Vec4i(x, y, width, height));
}

void GlScene::setBackgroundColor(const Color& color) {
  if (_backgroundColor != color) {
    _backgroundColor = color;
    _sceneNeedRedraw = true;
  }
}

void GlScene::initGlParameters(bool drawBackBufferBackup) {
  glViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
  glScissor(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
  if (!drawBackBufferBackup) {
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_STENCIL_TEST);
    glClearStencil(0xFF);
    glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    if (!_pickingMode) {
      glEnable(GL_BLEND);      
#ifdef __EMSCRIPTEN__
      // assuming we are blending a Tulip scene with DOM elemements under the rendering canvas,
      // need to use a blending function that works with premultiplied alpha
      if (_backgroundColor[3] != 255 || !_clearBufferAtDraw) {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
#else
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    } else {
      glDisable(GL_BLEND);
    }
  }
}

void GlScene::requestDraw() {
  sendEvent(GlSceneEvent(GlSceneEvent::DRAW_REQUEST, this));
}

void GlScene::draw() {

  BoundingBox sceneBoundingBox = getBoundingBox();
  if (_lastSceneBoundingBox[0] != sceneBoundingBox[0] || _lastSceneBoundingBox[1] != sceneBoundingBox[1]) {
    _lodCalculator->setSceneBoundingBox(sceneBoundingBox);
    _lastSceneBoundingBox = sceneBoundingBox;
  }

  if (_sceneNeedRedraw || _pickingMode) {

    if (_clearBufferAtDraw) {
      if (!_pickingMode) {
        glClearColor(_backgroundColor.getRGL(), _backgroundColor.getGGL(), _backgroundColor.getBGL(), _backgroundColor.getAGL());
      } else {
        glClearColor(0.f, 0.f, 0.f, 0.f);
      }
      glClear(GL_COLOR_BUFFER_BIT);
    }

    initGlParameters();

    for(vector<pair<string, GlLayer *> >::iterator it=_layersList.begin(); it!=_layersList.end(); ++it) {

      glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      Camera &camera = *(it->second->getCamera());
      Light &light = *(it->second->getLight());
      GlLODSceneVisitor glLODSceneVisitor(_lodCalculator);
      it->second->acceptVisitor(&glLODSceneVisitor);
      if (camera.is3d()) {
        glEnable(GL_DEPTH_TEST);
      } else {
        glDisable(GL_DEPTH_TEST);
      }
      camera.setViewport(_viewport);
      camera.initGl();
      if (!_pickingMode) {
        _lodCalculator->compute(&camera);
      } else {
        _lodCalculator->compute(&camera, _selectionViewport);
      }
      const vector<GlEntityLODUnit> &lodResult = _lodCalculator->getGlEntitiesResult(it->second);

      if (!lodResult.empty()) {
        for (size_t i = 0 ; i < lodResult.size() ; ++i) {
          if (lodResult[i].lod < 0 || !lodResult[i].glEntity->isVisible()) {
            continue;
          }
          glStencilFunc(GL_LEQUAL, lodResult[i].glEntity->getStencil(), 0xFF);
          lodResult[i].glEntity->draw(camera, light, _pickingMode);
        }
      }

    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);

    if (!_pickingMode && _backupBackBuffer && !GlFrameBufferObject::bufferBound()) {
      backupBackBuffer();
    }

  } else if (_backupBackBuffer && !GlFrameBufferObject::bufferBound()) {
    initGlParameters(true);
    drawBackBufferBackup();
  }

  if (!_pickingMode) {
    _sceneNeedRedraw = false;
  }
}

void GlScene::backupBackBuffer() {
  if (_backBufferTexture == 0) {
    glGenTextures(1, &_backBufferTexture);
    glBindTexture(GL_TEXTURE_2D, _backBufferTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    TextureManager::instance()->addExternalTexture(_backBufferTextureName, _backBufferTexture);
  }
  glBindTexture(GL_TEXTURE_2D, _backBufferTexture);
#ifdef __EMSCRIPTEN__
  glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _viewport[0], _viewport[1], _viewport[2], _viewport[3], 0);
#else
  // On desktop OpenGL, glCopyTexImage2D does not preserve antialiasing so we grab the back buffer content
  // and create a texture from it
  glReadBuffer(GL_BACK);
  glReadPixels(_viewport[0], _viewport[1], _viewport[2], _viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, _backBufferBackup);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _viewport[2], _viewport[3], 0, GL_RGBA, GL_UNSIGNED_BYTE, _backBufferBackup);
#endif
  glBindTexture(GL_TEXTURE_2D, 0);
  checkOpenGLError();
}

void GlScene::drawBackBufferBackup() {
  glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  Camera camera2d(false);
  camera2d.setViewport(_viewport);
  camera2d.initGl();
  Vec2f bl(0, 0);
  Vec2f tr(_viewport[2], _viewport[3]);
  GlRect2D rect(bl, tr, 0, Color::White);
  rect.setTexture(_backBufferTextureName);
  rect.draw(camera2d);
}

GlLayer *GlScene::createLayer(const string &name, bool is3d) {
  GlLayer *oldLayer = getLayer(name);

  if(oldLayer != NULL) {
    warning() << "Warning : You have a layer in the scene with same name, previous layer will be removed" << endl;
    removeLayer(oldLayer);
  }

  GlLayer *newLayer = NULL;
  if (!is3d) {
    newLayer = new GlLayer(name, is3d);
  } else {
    newLayer = new GlLayer(name, getMainLayer()->getCamera(), getMainLayer()->getLight());
  }
  _layersList.push_back(pair<string,GlLayer*>(name, newLayer));
  newLayer->setScene(this);

  sendEvent(GlSceneEvent(GlSceneEvent::LAYER_ADDED_IN_SCENE, this, newLayer));

  return newLayer;
}

GlLayer *GlScene::createLayerBefore(const string &layerName, const string &beforeLayerWithName, bool is3d) {
  GlLayer *newLayer = NULL;
  GlLayer *oldLayer = getLayer(layerName);

  for(vector<pair<string, GlLayer *> >::iterator it = _layersList.begin() ; it != _layersList.end() ; ++it) {
    if(it->first == beforeLayerWithName) {
      if (!is3d) {
        newLayer = new GlLayer(layerName, is3d);
      } else {
        newLayer = new GlLayer(layerName, getMainLayer()->getCamera(), getMainLayer()->getLight());
      }
      _layersList.insert(it, pair<string,GlLayer*>(layerName,newLayer));
      newLayer->setScene(this);

      if(oldLayer != NULL) {
        removeLayer(oldLayer);
        warning() << "Warning : You have a layer in the scene with same name, previous layer will be removed" << endl;
      }

      break;
    }
  }

  if (newLayer) {
    sendEvent(GlSceneEvent(GlSceneEvent::LAYER_ADDED_IN_SCENE, this, newLayer));
  }

  return newLayer;
}

GlLayer *GlScene::createLayerAfter(const string &layerName, const string &afterLayerWithName, bool is3d) {
  GlLayer *newLayer = NULL;
  GlLayer *oldLayer = getLayer(layerName);

  for(vector<pair<string, GlLayer *> >::iterator it = _layersList.begin() ; it != _layersList.end(); ++it) {
    if(it->first == afterLayerWithName) {
      ++it;
      if (!is3d) {
        newLayer = new GlLayer(layerName, is3d);
      } else {
        newLayer = new GlLayer(layerName, getMainLayer()->getCamera(), getMainLayer()->getLight());
      }
      _layersList.insert(it, pair<string,GlLayer*>(layerName, newLayer));
      newLayer->setScene(this);

      if(oldLayer != NULL) {
        warning() << "Warning : You have a layer in the scene with same name, previous layer will be removed" << endl;
        removeLayer(oldLayer);
      }

      break;
    }
  }

  if (newLayer) {
    sendEvent(GlSceneEvent(GlSceneEvent::LAYER_ADDED_IN_SCENE, this, newLayer));
  }

  return newLayer;
}

void GlScene::addExistingLayer(GlLayer *layer) {
  GlLayer *oldLayer=getLayer(layer->getName());

  if(oldLayer != NULL) {
    warning() << "Warning : You have a layer in the scene with same name, previous layer will be removed" << endl;
    removeLayer(oldLayer);
  }

  _layersList.push_back(pair<string,GlLayer*>(layer->getName(), layer));
  layer->setScene(this);
  _sceneNeedRedraw = true;

  sendEvent(GlSceneEvent(GlSceneEvent::LAYER_ADDED_IN_SCENE, this, layer));
  layerAddedInScene(layer);

}

bool GlScene::addExistingLayerBefore(GlLayer *layer, const string &beforeLayerWithName) {
  bool insertionOk = false;
  GlLayer *oldLayer = getLayer(layer->getName());

  for(vector<pair<string, GlLayer *> >::iterator it = _layersList.begin() ; it != _layersList.end() ; ++it) {
    if(it->first == beforeLayerWithName) {
      _layersList.insert(it, pair<string,GlLayer*>(layer->getName(),layer));
      layer->setScene(this);
      if(oldLayer != NULL) {
        warning() << "Warning : You have a layer in the scene with same name, previous layer will be removed" << endl;
        removeLayer(oldLayer);
      }
      insertionOk=true;
      _sceneNeedRedraw = true;

      sendEvent(GlSceneEvent(GlSceneEvent::LAYER_ADDED_IN_SCENE, this, layer));
      layerAddedInScene(layer);

      break;
    }
  }

  return insertionOk;
}

bool GlScene::addExistingLayerAfter(GlLayer *layer, const string &afterLayerWithName) {
  bool insertionOk=false;
  GlLayer *oldLayer=getLayer(layer->getName());

  for(vector<pair<string, GlLayer *> >::iterator it = _layersList.begin() ; it!=_layersList.end() ; ++it) {
    if(it->first == afterLayerWithName) {
      ++it;
      _layersList.insert(it,pair<string,GlLayer*>(layer->getName(),layer));
      layer->setScene(this);
      if(oldLayer != NULL) {
        warning() << "Warning : You have a layer in the scene with same name, previous layer will be removed" << endl;
        removeLayer(oldLayer);
      }
      insertionOk=true;
      _sceneNeedRedraw = true;

      sendEvent(GlSceneEvent(GlSceneEvent::LAYER_ADDED_IN_SCENE, this, layer));
      layerAddedInScene(layer);

      break;
    }
  }

  return insertionOk;
}

void GlScene::layerAddedInScene(GlLayer *layer) {
  const std::map<std::string, GlEntity*> &entities = layer->getGlEntities();
  std::map<std::string, GlEntity*>::const_iterator it = entities.begin();
  for (; it != entities.end() ; ++it) {
    _lodCalculator->addGlEntity(layer, it->second);
  }
}

GlLayer *GlScene::getLayer(const string &name) {
  for(vector<pair<string, GlLayer *> >::iterator it = _layersList.begin() ; it!=_layersList.end() ; ++it) {
    if(it->first == name) {
      return it->second;
    }
  }

  return NULL;
}

void GlScene::removeLayer(const string &name, bool deleteLayer) {
  for(vector<pair<string, GlLayer *> >::iterator it=_layersList.begin() ; it!=_layersList.end() ; ++it) {
    if(it->first == name) {
      _lodCalculator->removeLayer(it->second);
      _layersList.erase(it);
      _sceneNeedRedraw = true;

      sendEvent(GlSceneEvent(GlSceneEvent::LAYER_REMOVED_FROM_SCENE, this, it->second));

      if(deleteLayer)
        delete it->second;

      return;
    }
  }
}

void GlScene::removeLayer(GlLayer *layer, bool deleteLayer) {
  for(vector<pair<string, GlLayer *> >::iterator it = _layersList.begin() ; it != _layersList.end() ; ++it) {
    if (it->second == layer) {
      _lodCalculator->removeLayer(layer);
      _layersList.erase(it);
      _sceneNeedRedraw = true;

      sendEvent(GlSceneEvent(GlSceneEvent::LAYER_REMOVED_FROM_SCENE, this, it->second));

      if(deleteLayer)
        delete it->second;

      return;
    }
  }
}

void GlScene::clearLayersList() {
  for(vector<pair<string,GlLayer*> >::iterator it=_layersList.begin(); it!=_layersList.end(); ++it) {
    sendEvent(GlSceneEvent(GlSceneEvent::LAYER_REMOVED_FROM_SCENE, this, it->second));
    delete it->second;
  }
  _layersList.clear();
  _sceneNeedRedraw = true;
}

BoundingBox GlScene::getBoundingBox() {
  GlBoundingBoxSceneVisitor glBBSceneVisitor;
  for(vector<pair<string,GlLayer*> >::iterator it=_layersList.begin(); it!=_layersList.end(); ++it) {
    if (it->second->getCamera()->is3d()) {
      it->second->acceptVisitor(&glBBSceneVisitor);
    }
  }
  return glBBSceneVisitor.getBoundingBox();
}

void GlScene::centerScene(BoundingBox boundingBox) {
  BoundingBox sceneBB = boundingBox;
  if (!sceneBB.isValid()) {
    sceneBB = getBoundingBox();
  }
  for(vector<pair<string,GlLayer*> >::iterator it=_layersList.begin(); it!=_layersList.end(); ++it) {
    if (it->second->getCamera()->is3d() && !it->second->useSharedCamera()) {
      it->second->getCamera()->setViewport(_viewport);
      it->second->getCamera()->centerScene(sceneBB);
    }
  }
}

void GlScene::translate(int x, int y) {
  for(vector<pair<string,GlLayer*> >::iterator it=_layersList.begin(); it!=_layersList.end(); ++it) {
    Camera *camera = it->second->getCamera();
    if (camera->is3d() && !it->second->useSharedCamera()) {
      Coord v1(0, 0, 0);
      Coord v2(x, y, 0);
      v1 = camera->screenTo3DWorld(v1);
      v2 = camera->screenTo3DWorld(v2);
      Coord move = v2 - v1;
      camera->translate(move);
    }
  }
}

void GlScene::rotate(int x, int y, int z) {
  for(vector<pair<string,GlLayer*> >::iterator it=_layersList.begin(); it!=_layersList.end(); ++it) {
    Camera *camera = it->second->getCamera();
    if (camera->is3d() && !it->second->useSharedCamera()) {
      Coord center = getBoundingBox().center();
      camera->translate(center);
      camera->rotateX(static_cast<float>(x/360.0 * M_PI));
      camera->rotateY(static_cast<float>(y/360.0 * M_PI));
      camera->rotateZ(static_cast<float>(z/360.0 * M_PI));
      camera->translate(-center);
    }
  }
}

void GlScene::zoomXY(int x, int y, int step) {
  for(vector<pair<string,GlLayer*> >::iterator it=_layersList.begin(); it!=_layersList.end(); ++it) {
    Camera *camera = it->second->getCamera();
    if (camera->is3d() && !it->second->useSharedCamera()) {
      camera->setZoomFactor(camera->getZoomFactor() * pow(1.1,step));
    }
  }
  if (step < 0) step *= -1;
  int factX = static_cast<int>(step*(static_cast<double>(_viewport[2])/2.0-x)/ 7.0);
  int factY = static_cast<int>(step*(static_cast<double>(_viewport[3])/2.0-y)/ 7.0);
  translate(-factX,factY);
}

void GlScene::treatEvent(const Event &message) {
  if (message.type() != Event::TLP_MODIFICATION) return;
  Camera *camera = dynamic_cast<Camera*>(message.sender());
  GlEntity *entity = dynamic_cast<GlEntity*>(message.sender());
  GlLayer *layer = dynamic_cast<GlLayer*>(message.sender());
  if (camera || entity || layer) {
    _sceneNeedRedraw = true;
  }
  if (entity) {
    _lodCalculator->removeGlEntity(entity->getLayer(), entity);
  }
  const GlLayerEvent *layerEvt = dynamic_cast<const GlLayerEvent*>(&message);
  if (layerEvt) {
    if (layerEvt->getType() == GlLayerEvent::ENTITY_ADDED_IN_LAYER) {
      _lodCalculator->addGlEntity(layerEvt->getGlLayer(), layerEvt->getGlEntity());
    } else if (layerEvt->getType() == GlLayerEvent::ENTITY_REMOVED_FROM_LAYER) {
      _lodCalculator->removeGlEntity(layerEvt->getGlLayer(), layerEvt->getGlEntity());
    }
  }
}

bool GlScene::selectEntities(RenderingEntitiesFlag type, int x, int y, int width, int height, vector<SelectedEntity>& selectedEntities, GlLayer *layer) {
  _pickingMode = true;
  vector<SelectedEntity> selectedEntitiesInternal;
  selectedEntities.clear();

  x = clamp(x, 0, _viewport[2]);
  y = clamp(y, 0, _viewport[3]);
  if (x+width > _viewport[2]) {
    width -= x+width-_viewport[2];
  }
  if (y+height > _viewport[3]) {
    height -= y+height-_viewport[3];
  }

  GlFrameBufferObject *fbo = new GlFrameBufferObject(_viewport[2], _viewport[3], GlFrameBufferObject::NoAttachment);
  fbo->bind();
  unsigned int bufferSize = width*height*4;
  unsigned char *buffer = new unsigned char[bufferSize];
  bool done = false;
  _selectionViewport = Vec4i(x, y, width, height);

  std::vector<std::pair<std::string, GlLayer*> > layersList = getLayersList();
  std::map<GlLayer *, bool> layersVisibility;
  for (size_t i = 0 ; i < layersList.size() ; ++i) {
    layersVisibility[layersList[i].second] = layersList[i].second->isVisible();
  }

  if (layer && layer->getScene() == this) {
    for (size_t i = 0 ; i < layersList.size() ; ++i) {
      layersList[i].second->setVisible(false);
    }
    layer->setVisible(true);
  }

  while (!done) {
    set<GlEntity*> tmpSelectedEntities;
    draw();
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    for (unsigned int i = 0 ; i < bufferSize ; i += 4) {
      Color color(buffer[i], buffer[i+1], buffer[i+2], buffer[i+3]);
      unsigned int id = colorToUint(color);
      if (id == 0) continue;
      GlEntity *entity = GlEntity::fromId(id);
      if (entity) {
        tmpSelectedEntities.insert(entity);
        entity->setVisible(false);
      }
    }
    if (tmpSelectedEntities.empty()) {
      done = true;
    } else {
      for (set<GlEntity*>::iterator it = tmpSelectedEntities.begin() ; it != tmpSelectedEntities.end() ; ++it) {
        selectedEntitiesInternal.push_back(SelectedEntity(*it));
      }
    }
  }
  delete [] buffer;
  fbo->release();
  delete fbo;
  _pickingMode = false;

  for (size_t i = 0 ; i < selectedEntitiesInternal.size() ; ++i) {
    GlEntity *entity = selectedEntitiesInternal[i].getGlEntity();
    entity->setVisible(true);
    GlGraph *glGraph = dynamic_cast<GlGraph*>(entity);
    if (!glGraph && (type & RenderingGlEntities)) {
      selectedEntities.push_back(selectedEntitiesInternal[i]);
    } else if (glGraph && ((type & RenderingNodes) || (type & RenderingEdges))) {
      set<node> selectedNodes;
      set<edge> selectedEdges;
      Camera *camera = glGraph->getLayer()->getCamera();
      camera->initGl();
      glGraph->pickNodesAndEdges(*camera, x, y, width, height, selectedNodes, selectedEdges);
      if (!selectedNodes.empty() && (type & RenderingNodes)) {
        for (set<node>::iterator it = selectedNodes.begin() ; it != selectedNodes.end() ; ++it) {
          selectedEntities.push_back(SelectedEntity(SelectedEntity::NODE_SELECTED, glGraph, it->id));
        }
      }
      if (!selectedEdges.empty() && (type & RenderingEdges)) {
        for (set<edge>::iterator it = selectedEdges.begin() ; it != selectedEdges.end() ; ++it) {
          selectedEntities.push_back(SelectedEntity(SelectedEntity::EDGE_SELECTED, glGraph, it->id));
        }
      }
    }
  }

  if (layer && layer->getScene() == this) {
    for (size_t i = 0 ; i < layersList.size() ; ++i) {
      layersList[i].second->setVisible(layersVisibility[layersList[i].second]);
    }
  }

  return !selectedEntities.empty();
}

bool GlScene::selectEntity(RenderingEntitiesFlag type, int x, int y, SelectedEntity &selectedEntity, GlLayer *layer) {
  vector<SelectedEntity> selectedEntities;
  bool ret = selectEntities(type, x-1, y-1, 3, 3, selectedEntities, layer);
  if (!selectedEntities.empty()) {
    selectedEntity = selectedEntities.front();
  } else {
    selectedEntity = SelectedEntity();
  }
  return ret;
}

set<GlEntity*> GlScene::getEntities() {
  set<GlEntity*> ret;
  for(vector<pair<string,GlLayer*> >::iterator it=_layersList.begin() ; it!=_layersList.end() ; ++it) {
    GlLayer *layer = it->second;
    const map<string, GlEntity*> &layerEntities = layer->getGlEntities();
    for (map<string, GlEntity*>::const_iterator it2 = layerEntities.begin() ; it2 != layerEntities.end() ; ++it2) {
      GlEntity *entity = it2->second;
      ret.insert(entity);
    }
  }
  return ret;
}
