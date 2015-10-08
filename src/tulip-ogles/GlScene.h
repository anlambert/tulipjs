#ifndef GLSCENE_H
#define GLSCENE_H

#include <tulip/Vector.h>
#include <tulip/Color.h>
#include <tulip/BoundingBox.h>
#include <tulip/Observable.h>

#include <vector>

#include "GlLODCalculator.h"
#include "GlGraph.h"

class GlLayer;
class GlEntity;

namespace tlp {
class Graph;
}

struct SelectedEntity {

  enum SelectedEntityType {
    NO_ENTITY_SELECTED = 0,
    ENTITY_SELECTED = 1,
    NODE_SELECTED = 2,
    EDGE_SELECTED = 3
  };

  SelectedEntity() : _entityType(NO_ENTITY_SELECTED), _entity(NULL), _graphEltId(UINT_MAX) {}

  SelectedEntity(GlEntity *entity) : _entityType(ENTITY_SELECTED), _entity(entity), _graphEltId(UINT_MAX) {}

  SelectedEntity(SelectedEntityType type, GlGraph *glGraph, unsigned int id) : _entityType(type), _entity(glGraph), _graphEltId(id) {}

  SelectedEntityType getEntityType() const {
    return _entityType;
  }

  GlEntity *getGlEntity() const {
    return _entity;
  }

  GlGraph *getGlGraph() const {
    return dynamic_cast<GlGraph*>(_entity);
  }

  tlp::node getNode()const {
    return tlp::node(_graphEltId);
  }

  tlp::edge getEdge()const {
    return tlp::edge(_graphEltId);
  }

protected :

  SelectedEntityType _entityType;
  GlEntity *_entity;
  unsigned int _graphEltId;
};

class GlScene : public tlp::Observable {

public:

  GlScene();

  ~GlScene();

  void draw();

  void centerScene();

  void setViewport(const tlp::Vec4i &viewport);

  void setViewport(int x, int y, int width, int height);

  tlp::Vec4i getViewport() const {
    return _viewport;
  }

  void setBackgroundColor(const tlp::Color& color);

  tlp::Color getBackgroundColor() const {
    return _backgroundColor;
  }

  GlLayer *getMainLayer() const {
    return _mainLayer;
  }

  GlLayer *createLayer(const std::string &name, bool is3d = true);

  GlLayer *createLayerBefore(const std::string &layerName,const std::string &beforeLayerWithName, bool is3d = true);

  GlLayer *createLayerAfter(const std::string &layerName,const std::string &afterLayerWithName, bool is3d = true);

  void addExistingLayer(GlLayer *layer);

  bool addExistingLayerBefore(GlLayer *layer, const std::string &beforeLayerWithName);

  bool addExistingLayerAfter(GlLayer *layer, const std::string &afterLayerWithName);

  GlLayer *getLayer(const std::string& name);

  void removeLayer(const std::string& name, bool deleteLayer=false);

  void removeLayer(GlLayer *layer, bool deleteLayer=false);

  const std::vector<std::pair<std::string, GlLayer*> > &getLayersList() const {
    return _layersList;
  }

  void clearLayersList();

  tlp::BoundingBox getBoundingBox();

  void setClearBufferAtDraw(bool clear) {
    _clearBufferAtDraw = clear;
  }

  bool getClearBufferAtDraw() const {
    return _clearBufferAtDraw;
  }

  void translate(int x, int y);

  void rotate(int x, int y, int z);

  void zoomXY(int x, int y, int step);

  bool selectEntities(RenderingEntitiesFlag type, int x, int y, int width, int height, std::vector<SelectedEntity>& selectedEntities, GlLayer *layer = NULL);

  bool selectEntity(RenderingEntitiesFlag type, int x, int y, SelectedEntity &selectedEntity, GlLayer *layer = NULL);

  std::set<GlEntity*> getEntities();

  void forceRedraw() { _sceneNeedRedraw = true;}

  void initGlParameters(bool drawBackBufferBackup=false);

  void setBackupBackBuffer(bool backup) {_backupBackBuffer = backup;}

protected:

  void treatEvent(const tlp::Event &message);

private:

  void backupBackBuffer();
  void drawBackBufferBackup();

  GlLayer *_mainLayer;
  std::vector<std::pair<std::string, GlLayer *> > _layersList;
  GlLODCalculator *_lodCalculator;
  tlp::Vec4i _viewport, _selectionViewport;
  tlp::Color _backgroundColor;
  bool _clearBufferAtDraw;
  bool _sceneNeedRedraw;
  unsigned int _backBufferTexture;
  unsigned char *_backBufferBackup;

  bool _pickingMode;
  bool _backupBackBuffer;

};

class GlSceneEvent : public tlp::Event {
public:

  enum GlSceneEventType {LAYER_ADDED_IN_SCENE = 0,
                         LAYER_REMOVED_FROM_SCENE
                         };

  GlSceneEvent(GlSceneEventType type, GlScene *scene, GlLayer *layer,
               Event::EventType evtType = Event::TLP_MODIFICATION)

    : Event(*scene, evtType), _type(type), _scene(scene), _layer(layer) {}

  GlScene *getGlScene() const {
    return _scene;
  }

  GlLayer *getGlLayer() const {
    return _layer;
  }

  GlSceneEventType getType() const {
    return _type;
  }

protected:

  GlSceneEventType _type;
  GlScene *_scene;
  GlLayer *_layer;
};

#endif // GLSCENE_H
