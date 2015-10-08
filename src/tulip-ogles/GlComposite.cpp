#include "GlComposite.h"

using namespace std;
//============================================================
GlComposite::GlComposite(bool deleteEntitiesInDestructor):_deleteEntitiesInDestructor(deleteEntitiesInDestructor) {}
//============================================================
GlComposite::~GlComposite() {
  reset(_deleteEntitiesInDestructor);
}
//============================================================
void GlComposite::reset(bool deleteElems) {
  if (deleteElems) {
    std::map<string, GlEntity *>::const_iterator it;
    for(it = _entities.begin(); it != _entities.end(); ++it) {
      delete it->second;
    }
  }
  _entities.clear();
  notifyModified();
}
//============================================================
void GlComposite::addGlEntity(GlEntity *entity, const string &key) {
  _entities[key] = entity;
  entity->setVisible(_visible);
  notifyModified();
}
//============================================================
void GlComposite::deleteGlEntity(const string &key) {
  if(_entities.count(key)==0)
    return;
  _entities.erase(key);
  notifyModified();
}
//============================================================
void GlComposite::deleteGlEntity(GlEntity *entity) {
  std::map<string, GlEntity *>::const_iterator it;
  for(it = _entities.begin(); it != _entities.end(); ++it) {
    if(entity == (*it).second) {
      _entities.erase(it->first);
      notifyModified();
      return;
    }
  }
}
//============================================================
string GlComposite::findKey(GlEntity *entity) {
  std::map<string, GlEntity *>::const_iterator it;
  for(it = _entities.begin(); it != _entities.end(); ++it) {
    if(entity == (*it).second) {
      return it->first;
    }
  }
  return string("");
}
//============================================================
GlEntity* GlComposite::findGlEntity(const string &key) {
  std::map<string, GlEntity *>::const_iterator ite = _entities.find(key);
  if (ite == _entities.end())
    return NULL;
  return (*ite).second;
}
//============================================================
void GlComposite::setStencil(int stencil) {
  GlEntity::setStencil(stencil);
  std::map<string, GlEntity *>::const_iterator it;
  for(it = _entities.begin(); it != _entities.end(); ++it) {
    (*it).second->setStencil(stencil);
  }
  notifyModified();
}
//============================================================
void GlComposite::setVisible(bool visible) {
  GlEntity::setVisible(visible);
  std::map<string, GlEntity *>::const_iterator it;
  for(it = _entities.begin(); it != _entities.end(); ++it) {
    (*it).second->setVisible(visible);
  }
  notifyModified();
}
//============================================================
void GlComposite::acceptVisitor(GlSceneVisitor *visitor) {
  std::map<string, GlEntity *>::const_iterator it;
  for(it = _entities.begin(); it != _entities.end(); ++it) {
    if((*it).second->isVisible()) {
      (*it).second->acceptVisitor(visitor);
    }
  }
}
