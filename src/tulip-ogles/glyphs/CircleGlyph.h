#ifndef CIRCLEGLYPH_H
#define CIRCLEGLYPH_H

#include "Glyph.h"

class CircleGlyph : public Glyph {

public:

    CircleGlyph();

    virtual void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

};

#endif // CIRCLEGLYPH_H
