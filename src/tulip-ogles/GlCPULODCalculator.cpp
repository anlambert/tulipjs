#include "GlCPULODCalculator.h"
#include "Utils.h"
#include "glyphs/GlyphsManager.h"

#include <tulip/Matrix.h>
#include <tulip/Graph.h>
#include <tulip/LayoutProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/DoubleProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/IntegerProperty.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <algorithm>

using namespace std;
using namespace tlp;

class GlGraphRenderingParameters;

GlCPULODCalculator::GlCPULODCalculator() : _graph(NULL) {}

GlCPULODCalculator::~GlCPULODCalculator() {}

void GlCPULODCalculator::setGraph(tlp::Graph *graph, GlGraphRenderingParameters *renderingParameters) {
  clear();
  _graph = graph;
  _renderingParameters = renderingParameters;
  _viewLayout = graph->getProperty<LayoutProperty>("viewLayout");
  _viewSize = graph->getProperty<SizeProperty>("viewSize");
  _viewRotation = graph->getProperty<DoubleProperty>("viewRotation");
  _viewShape = graph->getProperty<IntegerProperty>("viewShape");
  _nodesLODVector.clear();
  _edgesLODVector.clear();
  _nodesLODVector.reserve(graph->numberOfNodes());
  _edgesLODVector.reserve(graph->numberOfEdges());
  _nodesBBCache.clear();
  _edgesBBCache.clear();
  node n;
  forEach(n, graph->getNodes()) {
    addNode(n);
  }
  edge e;
  forEach(e, graph->getEdges()) {
    addEdge(e);
  }
}

void GlCPULODCalculator::addGlEntity(GlLayer *layer, GlEntity *glEntity) {
  GlEntityLODUnit lodUnit(glEntity);
  if (find(_glEntitiesLODVector[layer].begin(), _glEntitiesLODVector[layer].end(), lodUnit) == _glEntitiesLODVector[layer].end()) {
    _glEntitiesLODVector[layer].push_back(lodUnit);
  }
}

void GlCPULODCalculator::removeGlEntity(GlLayer *layer, GlEntity *glEntity) {
  GlEntityLODUnit lodUnit(glEntity);
  vector<GlEntityLODUnit>::iterator it = find(_glEntitiesLODVector[layer].begin(), _glEntitiesLODVector[layer].end(), lodUnit);
  if (it != _glEntitiesLODVector[layer].end()) {
    _glEntitiesLODVector[layer].erase(it);
  }
}

void GlCPULODCalculator::addNode(const tlp::node n) {
  _nodesLODVector.push_back(NodeEntityLODUnit(n, getNodeBoundingBox(n)));
  _nodesBBCache[n] = _nodesLODVector.back().boundingBox;
}

void GlCPULODCalculator::addEdge(const tlp::edge e) {
  _edgesLODVector.push_back(EdgeEntityLODUnit(e, getEdgeBoundingBox(e)));
  _edgesBBCache[e] = _edgesLODVector.back().boundingBox;
}

BoundingBox GlCPULODCalculator::getNodeBoundingBox(node n) {
  double nodeRot = _viewRotation->getNodeValue(n);
  const Size& nodeSize = _viewSize->getNodeValue(n);
  const Coord& nodeCoord = _viewLayout->getNodeValue(n);

  if (nodeRot == 0.0) {
    BoundingBox box;
    box.expand(nodeCoord-nodeSize/2.f);
    box.expand(nodeCoord+nodeSize/2.f);
    return box;
  } else {
    float cosAngle=cos(nodeRot/180.*M_PI);
    float sinAngle=sin(nodeRot/180.*M_PI);
    Coord tmp1(nodeSize/2.f);
    Coord tmp2(tmp1[0] ,-tmp1[1], tmp1[2]);
    Coord tmp3(-tmp1[0],-tmp1[1],-tmp1[2]);
    Coord tmp4(-tmp1[0], tmp1[1],-tmp1[2]);
    tmp1=Coord(tmp1[0]*cosAngle-tmp1[1]*sinAngle,tmp1[0]*sinAngle+tmp1[1]*cosAngle,tmp1[2]);
    tmp2=Coord(tmp2[0]*cosAngle-tmp2[1]*sinAngle,tmp2[0]*sinAngle+tmp2[1]*cosAngle,tmp2[2]);
    tmp3=Coord(tmp3[0]*cosAngle-tmp3[1]*sinAngle,tmp3[0]*sinAngle+tmp3[1]*cosAngle,tmp3[2]);
    tmp4=Coord(tmp4[0]*cosAngle-tmp4[1]*sinAngle,tmp4[0]*sinAngle+tmp4[1]*cosAngle,tmp4[2]);
    BoundingBox bb;
    bb.expand(nodeCoord+tmp1);
    bb.expand(nodeCoord+tmp2);
    bb.expand(nodeCoord+tmp3);
    bb.expand(nodeCoord+tmp4);
    return bb;
  }
}

BoundingBox GlCPULODCalculator::getEdgeBoundingBox(edge e) {
  BoundingBox bb;
  const std::pair<node, node>& eEnds = _graph->ends(e);
  const node source = eEnds.first;
  const node target = eEnds.second;
  const Coord& srcCoord = _viewLayout->getNodeValue(source);
  const Coord& tgtCoord = _viewLayout->getNodeValue(target);

  const Size &srcSize = _viewSize->getNodeValue(source);
  const Size &tgtSize = _viewSize->getNodeValue(target);
  double srcRot = _viewRotation->getNodeValue(source);
  double tgtRot = _viewRotation->getNodeValue(target);

  const LineType::RealType &bends = _viewLayout->getEdgeValue(e);

  // set srcAnchor, tgtAnchor. tmpAnchor will be on the point just before tgtAnchor
  Coord srcAnchor, tgtAnchor, tmpAnchor;

  int srcGlyphId = _viewShape->getNodeValue(source);
  Glyph *sourceGlyph = GlyphsManager::getGlyph(srcGlyphId);
  tmpAnchor = (bends.size() > 0) ? bends.front() : tgtCoord;
  srcAnchor = sourceGlyph->getAnchor(srcCoord, tmpAnchor, srcSize, srcRot);

  int tgtGlyphId = _viewShape->getNodeValue(target);
  Glyph *targetGlyph = GlyphsManager::getGlyph(tgtGlyphId);
  //this time we don't take srcCoord but srcAnchor to be oriented to where the line comes from
  tmpAnchor = (bends.size() > 0) ? bends.back() : srcAnchor;
  tgtAnchor = targetGlyph->getAnchor(tgtCoord, tmpAnchor, tgtSize, tgtRot);

  if (!bends.empty()) {

    for (vector<Coord>::const_iterator it = bends.begin(); it != bends.end(); ++it)
      bb.expand(*it);
  }

  bb.expand(srcAnchor);
  bb.expand(tgtAnchor);

  return bb;
}

void GlCPULODCalculator::compute(Camera *camera, const Vec4i &selectionViewport) {

  this->camera = camera;
  Vec4i currentViewport = selectionViewport;
  if (currentViewport[0] < 0) {
    currentViewport = camera->getViewport();
  }

  if(camera->is3d()) {
    Coord eye=camera->getEyes() + (camera->getEyes() - camera->getCenter()) / static_cast<float>(camera->getZoomFactor());
    computeFor3DCamera(eye, camera->transformMatrix(), camera->getViewport(), currentViewport);
  }
  else {
    computeFor2DCamera(camera->getViewport(), currentViewport);
  }

}

void GlCPULODCalculator::computeFor3DCamera(const Coord &eye,
                                            const Matrix<float, 4> &transformMatrix,
                                            const Vector<int,4> &globalViewport,
                                            const Vector<int,4> &currentViewport) {

#ifdef _OPENMP
  omp_set_num_threads(omp_get_num_procs());
  omp_set_nested(true);
  omp_set_dynamic(false);
#endif

  size_t nb=0;
  if((_renderingEntitiesFlag & RenderingGlEntities)!=0) {
    map<GlLayer*, std::vector<GlEntityLODUnit> >::iterator it = _glEntitiesLODVector.begin();
    for (; it != _glEntitiesLODVector.end() ; ++it) {
      nb=it->second.size();
#ifdef _OPENMP
#pragma omp parallel for
#endif

      for(int i = 0 ; i < static_cast<int>(nb) ; ++i) {
        _glEntitiesLODVector[it->first][i].lod = calculateAABBSize(_glEntitiesLODVector[it->first][i].boundingBox, eye, transformMatrix, globalViewport, currentViewport);
      }
    }
  }

  if(_graph && (_renderingEntitiesFlag & RenderingNodes)!=0) {
    nb=_nodesLODVector.size();
#ifdef _OPENMP
#pragma omp parallel for
#endif

    for(int i = 0 ; i < static_cast<int>(nb) ; ++i) {
      _nodesLODVector[i].lod = calculateAABBSize(_nodesLODVector[i].boundingBox, eye, transformMatrix, globalViewport, currentViewport);
    }

    _edgeSizeLodCache.clear();
  }

  if(_graph && (_renderingEntitiesFlag & RenderingEdges)!=0) {
    nb=_edgesLODVector.size();

#ifdef _OPENMP
#pragma omp parallel for
#endif

    for(int i = 0 ; i < static_cast<int>(nb) ; ++i) {
      _edgesLODVector[i].lod = calculateAABBSize(_edgesLODVector[i].boundingBox, eye, transformMatrix, globalViewport, currentViewport);
      if (_edgesLODVector[i].lod > 0) {

        Size edgeSize = getEdgeSize(_graph, _edgesLODVector[i].e, _viewSize, _renderingParameters);

        if (_edgeSizeLodCache.find(edgeSize) != _edgeSizeLodCache.end()) {
          _edgesLODVector[i].lodSize = _edgeSizeLodCache[edgeSize];
          continue;
        }

        const Coord &edgeCoord = _viewLayout->getNodeValue(_graph->source(_edgesLODVector[i].e));

        if (edgeSize[0] != edgeSize[1]) {
          _edgesLODVector[i].lodSize = std::max(std::abs(projectSize(edgeCoord, Size(edgeSize[0], edgeSize[0], edgeSize[0]), camera->projectionMatrix(), camera->modelviewMatrix(), camera->getViewport())),
              std::abs(projectSize(edgeCoord, Size(edgeSize[1], edgeSize[1], edgeSize[1]), camera->projectionMatrix(), camera->modelviewMatrix(), camera->getViewport())));
        }
        else {
          _edgesLODVector[i].lodSize = std::abs(projectSize(edgeCoord, Size(edgeSize[0], edgeSize[0], edgeSize[0]), camera->projectionMatrix(), camera->modelviewMatrix(), camera->getViewport()));
        }
#pragma omp critical
        _edgeSizeLodCache[edgeSize] = _edgesLODVector[i].lodSize;
      }
    }
  }
}

void GlCPULODCalculator::computeFor2DCamera(const Vector<int,4> &globalViewport,
                                            const Vector<int,4> &currentViewport) {

  if((_renderingEntitiesFlag & RenderingGlEntities)!=0) {
    map<GlLayer*, std::vector<GlEntityLODUnit> >::iterator it = _glEntitiesLODVector.begin();
    for (; it != _glEntitiesLODVector.end() ; ++it) {
      for(vector<GlEntityLODUnit>::iterator it2=it->second.begin(); it2!=it->second.end(); ++it2) {
        (*it2).lod=calculate2DLod((*it2).boundingBox, globalViewport, currentViewport);
      }
    }
  }

  if(_graph && (_renderingEntitiesFlag & RenderingNodes)!=0) {
    for(vector<NodeEntityLODUnit>::iterator it=_nodesLODVector.begin(); it!=_nodesLODVector.end(); ++it) {
      (*it).lod=calculate2DLod((*it).boundingBox, globalViewport, currentViewport);
    }
  }

  if(_graph && (_renderingEntitiesFlag & RenderingNodes)!=0) {
    for(vector<EdgeEntityLODUnit>::iterator it=_edgesLODVector.begin(); it!=_edgesLODVector.end(); ++it) {
      (*it).lod=calculate2DLod((*it).boundingBox, globalViewport, currentViewport);
    }
  }
}

