#ifndef PENTAGONGLYPH_H
#define PENTAGONGLYPH_H

#include "Glyph.h"

class PentagonGlyph : public Glyph {

public:

    PentagonGlyph();

    virtual void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

};

#endif // PENTAGONGLYPH_H
