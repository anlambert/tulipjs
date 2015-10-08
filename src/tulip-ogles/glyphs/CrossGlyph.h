#ifndef CROSSGLYPH_H
#define CROSSGLYPH_H

#include "Glyph.h"

class CrossGlyph : public Glyph {

public:

    CrossGlyph();

    void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

protected:

    tlp::Coord getAnchor(const tlp::Coord &vector) const;
};

#endif // CROSSGLYPH_H
