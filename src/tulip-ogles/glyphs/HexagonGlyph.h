#ifndef HEXAGONGLYPH_H
#define HEXAGONGLYPH_H

#include "Glyph.h"

class HexagonGlyph : public Glyph {

public:

    HexagonGlyph();

    virtual void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

};

#endif // HEXAGONGLYPH_H
