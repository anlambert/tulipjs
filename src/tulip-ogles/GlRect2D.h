#ifndef GLRECT_H
#define GLRECT_H

#include "GlConvexPolygon.h"

class GlRect2D : public GlConvexPolygon {

public:

    GlRect2D(const tlp::Vec2f &bottomLeftCorner, const tlp::Vec2f &topRightCorner, const float z, const tlp::Color &fillColor);

    GlRect2D(const tlp::Vec2f &bottomLeftCorner, const tlp::Vec2f &topRightCorner, const float z, const tlp::Color &fillColor, const tlp::Color &outlineColor);

private:

    void createContour(const tlp::Vec2f &bottomLeftCorner, const tlp::Vec2f &topRightCorner, const float z);


};

#endif // GLRECT_H
