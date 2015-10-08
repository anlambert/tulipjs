#ifndef GLSCENEVISITOR_H
#define GLSCENEVISITOR_H

class GlEntity;
class GlLayer;

class GlSceneVisitor {

public:

  GlSceneVisitor() {}
  virtual ~GlSceneVisitor() {}

  virtual void visit(GlEntity *) {}
  virtual void visit(GlLayer *) {}

};

#endif // GLSCENEVISITOR_H

