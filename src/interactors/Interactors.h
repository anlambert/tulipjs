#ifndef INTERACTORS_H
#define INTERACTORS_H

#include "ZoomAndPanInteractor.h"
#include "RectangleZoomInteractor.h"
#include "SelectionInteractor.h"
#include "SelectionModifierInteractor.h"
#include "NeighborhoodInteractor.h"
#include "LassoSelectionInteractor.h"

static ZoomAndPanInteractor zoomAndPanInteractor;
static RectangleZoomInteractor rectangleZoomInteractor;
static SelectionInteractor selectionInteractor;
static SelectionModifierInteractor selectionModifierInteractor;
static NeighborhoodInteractor neighborhoodInteractor;
static LassoSelectionInteractor lassoSelectionInteractor;

static std::map<std::string, GlSceneInteractor *> initInteractorsMap() {
  std::map<std::string, GlSceneInteractor *> ret;
  ret["ZoomAndPan"] = &zoomAndPanInteractor;
  ret["RectangleZoom"] = &rectangleZoomInteractor;
  ret["Selection"] = &selectionInteractor;
  ret["SelectionModifier"] = &selectionModifierInteractor;
  ret["Neighborhood"] = &neighborhoodInteractor;
  ret["LassoSelection"] = &lassoSelectionInteractor;
  return ret;
}

static std::map<std::string, GlSceneInteractor *> interactorsMap = initInteractorsMap();

#endif // INTERACTORS_H
