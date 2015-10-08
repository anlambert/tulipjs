#ifndef GLLAYER_H
#define GLLAYER_H

#include "GlComposite.h"

class GlScene;
class GlSceneVisitor;

class GlLayer : public tlp::Observable {

  friend class GlScene;

public:

  GlLayer(const std::string& name, bool is3d = true);

  GlLayer(const std::string& name, Camera *camera, Light *light);

  ~GlLayer();

  GlScene *getScene() {
    return _scene;
  }

  std::string getName() {
    return _name;
  }

  void setCamera(Camera *camera);

  void setSharedCamera(Camera *camera);

  Camera *getCamera() {
    return _camera;
  }

  void setLight(Light *light);

  void setSharedLight(Light *light);

  Light *getLight() {
    return _light;
  }

  void setVisible(bool visible);

  bool isVisible() {
    return _composite.isVisible();
  }

  void addGlEntity(GlEntity *entity, const std::string &_name);

  void deleteGlEntity(const std::string &key);

  void deleteGlEntity(GlEntity *entity);

  GlEntity* findGlEntity(const std::string &key);

  const std::map<std::string, GlEntity*> &getGlEntities() const;

  void clear(bool deleteEntities=false);

  bool useSharedCamera() {
    return _sharedCamera;
  }

  void acceptVisitor(GlSceneVisitor *visitor);

protected :

  void setScene(GlScene *_scene);

private:

  std::string _name;

  GlComposite _composite;
  GlScene *_scene;

  Camera *_camera;
  Light *_light;
  bool _sharedCamera;
  bool _sharedLight;

};

class GlLayerEvent : public tlp::Event {
public:

  enum GlLayerEventType {ENTITY_ADDED_IN_LAYER = 0,
                         ENTITY_REMOVED_FROM_LAYER
                         };

  GlLayerEvent(GlLayerEventType type, GlLayer *layer, GlEntity *entity,
                Event::EventType evtType = Event::TLP_MODIFICATION)

    : Event(*layer, evtType), _type(type), _layer(layer), _entity(entity) {}

  GlLayer *getGlLayer() const {
    return _layer;
  }

  GlEntity *getGlEntity() const {
    return _entity;
  }

  GlLayerEventType getType() const {
    return _type;
  }

protected:

  GlLayerEventType _type;
  GlLayer *_layer;
  GlEntity *_entity;
};

#endif // Tulip_GLLAYER_H
