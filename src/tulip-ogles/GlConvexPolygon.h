#ifndef GLCONVEXPOLYGON_H
#define GLCONVEXPOLYGON_H

#include <GL/gl.h>

#include <vector>

#include <tulip/Coord.h>
#include <tulip/Color.h>
#include <tulip/BoundingBox.h>

#include "GlEntity.h"

class GlBuffer;

class GlConvexPolygon : public GlEntity {

public:

    GlConvexPolygon();

    GlConvexPolygon(const std::vector<tlp::Coord> &contour, const tlp::Color &fillColor);

    GlConvexPolygon(const std::vector<tlp::Coord> &contour, const tlp::Color &fillColor, const tlp::Color &outlineColor, const float outlineWidth=1.f);

    ~GlConvexPolygon();

    void setPolygonContour(const std::vector<tlp::Coord> &contour);

    const std::vector<tlp::Coord> &polygonContour() const {
      return _contour;
    }

    void setFilled(const bool filled) {
        _filled = filled;
    }

    void setOutlined(const bool outlined) {
        _outlined = outlined;
    }

    void setFillColor(const tlp::Color &fillColor) {
        _fillColor = fillColor;
    }

    void setOutlineColor(const tlp::Color &outlineColor) {
        _outlineColor = outlineColor;
    }

    void setOutlineWidth(const float outlineWidth) {
      _outlineWidth = outlineWidth;
    }

    void setTexture(const std::string &texture) {
      _texture = texture;
    }

    void draw(const Camera &camera, bool pickingMode=false);

    void draw(const Camera &camera, const Light &, bool pickingMode=false);

    void translate(const tlp::Coord &move);

private:

    std::vector<tlp::Coord> _contour;

    bool _filled;
    bool _outlined;
    tlp::Color _fillColor;
    tlp::Color _outlineColor;
    float _outlineWidth;

    GlBuffer *_polygonDataBuffer;
    GlBuffer *_polygonIndicesBuffer;

    unsigned int _nbVertices;
    unsigned int _nbIndices;

    std::string _texture;

};

#endif // GLCONVEXPOLYGON_H
