#ifndef CONCAVEHULLBUILDER_H
#define CONCAVEHULLBUILDER_H

#include <tulip/Coord.h>

#include <vector>

#include "GlConcavePolygon.h"

namespace tlp {
class Graph;
}

class ConcaveHullBuilder {

public:

    ConcaveHullBuilder();

    void setScalingFactor(const float scalingFactor);

    void setHullWithHoles(const bool hullWithHoles);

    void setHullZValue(const float z);

    void addPolygon(const std::vector<tlp::Coord> &polygon);

    void computeHulls();

    unsigned int nbComputedHulls() const;

    const std::vector<std::vector<tlp::Coord> > &getHullWithHoles(unsigned int hullId) const;

    const std::vector<tlp::Coord> &getHullOuterContour(unsigned int hullId) const;

    void clear();

private:

    float _scalingFactor;

    bool _hullWithHoles;

    float _z;

    std::vector<std::vector<tlp::Coord> > _polygons;

    std::vector<std::vector<std::vector<tlp::Coord> > > _computedHulls;

};

GlConcavePolygon* computeGraphHull(tlp::Graph *graph, const tlp::Color &fillColor, const float z, const bool withHoles = true, const float spacing = 0.1f);

#endif // CONCAVEHULLBUILDER_H
