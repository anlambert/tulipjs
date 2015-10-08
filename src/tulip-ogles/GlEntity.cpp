#include "GlEntity.h"
#include "Utils.h"

using namespace std;
using namespace tlp;

unsigned int GlEntity::nextId(0);
map<unsigned int, GlEntity*> GlEntity::idToEntity;

GlEntity::GlEntity() : _visible(true), _stencil(0xFF), _selected(false), _layer(NULL) {
  _id = ++nextId;
  idToEntity[_id] = this;
  _pickingColor = uintToColor(_id);
  _boundingBox[0].fill(0);
  _boundingBox[1].fill(0);
}

void GlEntity::setVisible(bool visible) {
  if (_visible != visible) {
    _visible = visible;
    notifyModified();
  }
}

void GlEntity::setStencil(int stencil) {
  if (_stencil != stencil) {
    _stencil = stencil;
    notifyModified();
  }
}

void GlEntity::notifyModified() {
  sendEvent(Event(*this, Event::TLP_MODIFICATION));
}
