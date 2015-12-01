#ifndef SELECTIONINTERACTOR_H
#define SELECTIONINTERACTOR_H

#include "GlSceneInteractor.h"
#include "GlScene.h"

#include <vector>

class ZoomAndPanInteractor;

class SelectionInteractor : public GlSceneInteractor {

public:

  SelectionInteractor(GlScene *scene = NULL);

  void setScene(GlScene *glScene);

  virtual bool mouseCallback(const MouseButton &button, const MouseButtonState &state, int x, int y, const int &modifiers);

  virtual bool mouseMoveCallback(int x, int y, const int &modifiers);

  virtual void draw();

  void setSelectOnlyEdgesConnectedToSelectedNodes(bool state) {
    _selectOnlyEdgesConnectedToSelectedNodes = state;
  }

private:

  MouseButton _mouseButton;
  int _firstX, _firstY;
  int _curX, _curY;
  bool _dragStarted;

  ZoomAndPanInteractor *_znpInteractor;
  SelectedEntity _selectedEntity;
  std::vector<SelectedEntity> _selectedEntities;

  bool _selectOnlyEdgesConnectedToSelectedNodes;

};

#endif // SELECTIONINTERACTOR_H
