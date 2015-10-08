#include "GlLayer.h"
#include "Camera.h"
#include "GlSceneVisitor.h"
#include "GlScene.h"

using namespace std;

GlLayer::GlLayer(const std::string& name, bool is3d)
  :_name(name),_scene(0),_camera(new Camera(is3d)),_light(new Light(_camera)),_sharedCamera(false),_sharedLight(false) {}

GlLayer::GlLayer(const std::string& name, Camera *camera, Light *light)
  :_name(name),_scene(0),_camera(camera),_light(light),_sharedCamera(true),_sharedLight(true) {}

GlLayer::~GlLayer() {
  if(!_sharedCamera)
    delete _camera;
  if (!_sharedLight)
    delete _light;
}

void GlLayer::setScene(GlScene *scene) {
  if (_scene) {
    removeListener(_scene);
    _camera->removeListener(_scene);
    _composite.removeListener(_scene);
    std::map<std::string, GlEntity*>::const_iterator it = getGlEntities().begin();
    for (; it != getGlEntities().end() ; ++it) {
      it->second->removeListener(_scene);
    }
  }
  if (scene) {
    _scene=scene;
    addListener(_scene);
    _camera->addListener(_scene);
    _composite.addListener(_scene);
    std::map<std::string, GlEntity*>::const_iterator it = getGlEntities().begin();
    for (; it != getGlEntities().end() ; ++it) {
      it->second->addListener(_scene);
    }
  }
}

void GlLayer::setCamera(Camera *camera) {
  Camera *oldCamera = _camera;
  _camera = camera;

  if (!_sharedCamera)
    delete oldCamera;

  _sharedCamera = false;
  setScene(_scene);
}

void GlLayer::setSharedCamera(Camera *camera) {
  Camera *oldCamera = _camera;
  _camera = camera;

  if (!_sharedCamera)
    delete oldCamera;

  _sharedCamera = true;
  setScene(_scene);
}

void GlLayer::setLight(Light *light) {
  Light *oldLight = _light;
  _light = light;

  if (!_sharedLight)
    delete oldLight;

  _sharedLight = false;
}

void GlLayer::setSharedLight(Light *light) {
  Light *oldLight = _light;
  _light = light;

  if (!_sharedLight)
    delete oldLight;

  _sharedLight = true;
}

void GlLayer::setVisible(bool visible) {
  _composite.setVisible(visible);
}

void GlLayer::acceptVisitor(GlSceneVisitor *visitor) {
  assert(visitor);
  visitor->visit(this);
  if(_composite.isVisible()) {
    _composite.acceptVisitor(visitor);
  }
}

void GlLayer::addGlEntity(GlEntity *entity, const std::string& name) {
  assert(entity);
  entity->setLayer(this);
  _composite.addGlEntity(entity,name);
  if (_scene) {
    entity->addListener(_scene);
  }
  sendEvent(GlLayerEvent(GlLayerEvent::ENTITY_ADDED_IN_LAYER, this, entity));
}

void GlLayer::deleteGlEntity(const std::string &key) {
  GlEntity *entity = _composite.findGlEntity(key);
  if (entity) {
    if (_scene) {
      entity->removeListener(_scene);
    }
    sendEvent(GlLayerEvent(GlLayerEvent::ENTITY_REMOVED_FROM_LAYER, this, entity));
    entity->setLayer(NULL);
    _composite.deleteGlEntity(key);
  }
}

void GlLayer::deleteGlEntity(GlEntity *entity) {
  assert(entity);
  if (_scene) {
    entity->removeListener(_scene);
  }
  sendEvent(GlLayerEvent(GlLayerEvent::ENTITY_REMOVED_FROM_LAYER, this, entity));
  entity->setLayer(NULL);
  _composite.deleteGlEntity(entity);
}

void GlLayer::clear(bool deleteEntities) {
  if (_scene) {
    std::map<std::string, GlEntity*>::const_iterator it = getGlEntities().begin();
    for (; it != getGlEntities().end() ; ++it) {
      sendEvent(GlLayerEvent(GlLayerEvent::ENTITY_REMOVED_FROM_LAYER, this, it->second));
      it->second->removeListener(_scene);
    }
  }
  _composite.reset(deleteEntities);
}

GlEntity* GlLayer::findGlEntity(const std::string &key) {
  return _composite.findGlEntity(key);
}

const std::map<std::string, GlEntity*> &GlLayer::getGlEntities() const {
  return _composite.getGlEntities();
}
