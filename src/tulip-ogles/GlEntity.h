#ifndef GLENTITY_H
#define GLENTITY_H

#include <vector>
#include <map>

#include <tulip/BoundingBox.h>
#include <tulip/Observable.h>

#include "GlEntity.h"
#include "Light.h"
#include "GlSceneVisitor.h"

class GlEntity : public tlp::Observable {

public:

  GlEntity();

  virtual ~GlEntity();

  virtual void setVisible(bool visible);

  virtual bool isVisible() const {
    return _visible;
  }

  virtual void setStencil(int stencil);

  virtual int getStencil() {
    return _stencil;
  }

  virtual void draw(const Camera &camera, bool pickingMode=false) {
    static Light light;
    draw(camera, light, pickingMode);
  }

  virtual void draw(const Camera &camera, const Light &light, bool pickingMode=false) = 0;

  virtual tlp::BoundingBox getBoundingBox() {
    return _boundingBox;
  }

  virtual void acceptVisitor(GlSceneVisitor *visitor) {
    visitor->visit(this);
  }

  void setLayer(GlLayer *layer) {
    _layer = layer;
  }

  GlLayer *getLayer() const {
    return _layer;
  }

  void setSelected(bool selected) {
    _selected = selected;
  }

  bool selected() const {
    return _selected;
  }

  unsigned int getId() const {
    return _id;
  }

  static GlEntity *fromId(unsigned int id) {
    return idToEntity[id];
  }

protected:

  void notifyModified();

  bool _visible;
  int _stencil;
  bool _selected;

  tlp::BoundingBox _boundingBox;

  GlLayer *_layer;

  unsigned int _id;
  tlp::Color _pickingColor;

  static unsigned int nextId;
  static std::map<unsigned int, GlEntity*> idToEntity;

};

#endif // GLENTITY_H


