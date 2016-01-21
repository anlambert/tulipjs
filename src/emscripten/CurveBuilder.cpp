/**
 *
 * This file is part of Tulip (www.tulip-software.org)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux 1 and Inria Bordeaux - Sud Ouest
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */
#include "CurveBuilder.h"
#include <map>
#include <tulip/PluginProgress.h>
#include <tulip/Graph.h>
#include <tulip/LayoutProperty.h>
#include <tulip/SizeProperty.h>

#include <tulip/ParametricCurves.h>
#include <tulip/StringCollection.h>

PLUGIN(CurveBuilder)

using namespace tlp;

#define CURVE_TYPES "Bezier;Catmull-Rom;Cubic B-Spline"

CurveBuilder::CurveBuilder(tlp::PluginContext* context): LayoutAlgorithm(context) {
  addInParameter<LayoutProperty>("original layout", "Original layout values to compute curves from", "viewLayout", true);
  addInParameter<StringCollection>("curve type", "Type of curves to generate", CURVE_TYPES);
}

CurveBuilder::~CurveBuilder() {}

using namespace std;

class CellCBuilder {
public:
    CellCBuilder(){}
    CellCBuilder(const std::vector<tlp::Coord> *data, int id, int next, int prev):
    _data(data), _id(id) {
        set(prev, next);
    }
    const std::vector<tlp::Coord> *_data;
    int _id;
    int  _next, _prev;
    double _sin;

    void set(int prev, int next) {
        _prev = prev;
        _next = next;
        if (prev > -1 && next < (int) _data->size()) {
            Vec3f v1( (*_data)[_prev] - (*_data)[_id] );
            Vec3f v2( (*_data)[_next] - (*_data)[_id] );
            v1.normalize();
            v2.normalize();
            _sin = abs((v1^v2)[2]);
        }
        else
            _sin = 0; //we can't compute sin for extremities.
    }
    void setNext(int next) {
        set(_prev, next);
    }
    void setPrev(int prev) {
        set(prev, _next);
    }

    bool operator()(const CellCBuilder &a, const CellCBuilder &b) const {
        if (a._sin < b._sin) return true;
        if (a._sin > b._sin) return false;
        return a._id < b._id;
    }
};


std::vector<tlp::Coord> superSimplify(std::vector<tlp::Coord> &in) {
    vector<CellCBuilder> cells;
    for(unsigned int i=0; i < in.size() ; ++i) {
        cells.push_back(CellCBuilder(&in, i, i+1, i-1));
    }
    set<CellCBuilder, CellCBuilder> order;
    for(unsigned int i=1; i < in.size()-1 ; ++i) {
        order.insert(cells[i]);
    }
    set<CellCBuilder, CellCBuilder>::iterator it;

    const unsigned int    MAX_BENDS = 10; // we remove bends until there is atmost MAX_BENDS
    const double MIN_DEV  = sin(M_PI/ 360.); //0.5 degree, if there only MIN_DEV we remove the bend even if nbBends < MAX_BENDS
    const double MAX_DEV  = sin(M_PI/ 36. ); //5 degree, if there is more than MAX_DEV degree wee keep the bend even if nbBends > MAX_BEND;

    while( (order.size() > 0) &&
           ((order.begin()->_sin) < MAX_DEV) &&
           ((order.size() > MAX_BENDS) || ((order.begin())->_sin < MIN_DEV)) ){
        it = order.begin();
        int prev = it->_prev;
        int next = it->_next;
        order.erase(it);

        if (prev > 0) {
            order.erase(cells[prev]);
            cells[prev].setNext(next);
            order.insert(cells[prev]);
        }
        if (next < ((int)in.size())-1) {
            order.erase(cells[next]);
            cells[next].setPrev(prev);
            order.insert(cells[next]);
        }
    }

    std::vector<tlp::Coord> result;
    result.push_back(in[0]);
    for(unsigned int i=1; i < in.size() - 1 ; ++i) {
        if (order.find(cells[i]) != order.end())
            result.push_back(in[i]);
    }
    result.push_back(in[in.size()-1]);
    return result;
}

bool CurveBuilder::run() {
  LayoutProperty* origLayout = graph->getProperty<LayoutProperty>("viewLayout");
  LayoutProperty  *l = graph->getProperty<LayoutProperty>("viewLayout");
  SizeProperty    *s = graph->getProperty<SizeProperty>("viewSize");
  StringCollection curveType(CURVE_TYPES);
  if (dataSet->exist("curve type"))
    dataSet->get("curve type",curveType);
  if (dataSet->exist("original layout")) {
    dataSet->get("original layout",origLayout);
  }
  result->copy(origLayout);

  int curveIndex = curveType.getCurrent();

  edge ee;
  const int NB_POINTS = 200;
  std::vector<edge> edges;
  edges.reserve(graph->numberOfEdges());
  forEach(ee, graph->getEdges()) {
      edges.push_back(ee);
  }

  for (size_t i = 0 ; i < edges.size() ; ++i) {
    edge e = edges[i];
    Coord posSrc = l->getNodeValue(graph->source(e));
    Coord posTgt = l->getNodeValue(graph->target(e));
    Size  s1 = s->getNodeValue(graph->source(e));
    Size  s2 = s->getNodeValue(graph->target(e));

    std::vector<Coord> bends = result->getEdgeValue(e);
    bends.insert(bends.begin(),result->getNodeValue(graph->source(e)));
    bends.insert(bends.end(),result->getNodeValue(graph->target(e)));

    std::vector<tlp::Coord> curvePoints;
    if (curveIndex==0 && bends.size()>1)
      computeBezierPoints(bends,curvePoints, NB_POINTS);
    else if (curveIndex==1 && bends.size()>2)
      computeCatmullRomPoints(bends,curvePoints,false, NB_POINTS);
    else
      computeOpenUniformBsplinePoints(bends,curvePoints,3, NB_POINTS);
    {
        std::vector<tlp::Coord> resultPoints;
        resultPoints.push_back(curvePoints[0]);
        for(unsigned int i=1; i < curvePoints.size() - 1 ; ++i) {
            if (curvePoints[i].dist(posSrc) < s1.norm()/3.) continue;
            if (curvePoints[i].dist(posTgt) < s2.norm()/3.) continue;
            resultPoints.push_back(curvePoints[i]);
        }
        resultPoints.push_back(curvePoints[curvePoints.size()-1]);
        curvePoints = resultPoints;
    }

    curvePoints = superSimplify(curvePoints);

    std::vector<tlp::Coord> resultPoints;
    for(unsigned int i=1; i < curvePoints.size() - 1 ; ++i) {
        if (curvePoints[i].dist(posSrc) < s1.norm()/3.) continue;
        if (curvePoints[i].dist(posTgt) < s2.norm()/3.) continue;
        resultPoints.push_back(curvePoints[i]);
    }

    {
        result->setEdgeValue(e,resultPoints);
    }

  }

  return true;
}

bool CurveBuilder::check(std::string &msg) {
  msg="";
  return true;
}

