#ifndef DIAMONDGLYPH_H
#define DIAMONDGLYPH_H

#include "Glyph.h"

class DiamondGlyph : public Glyph {

public:

    DiamondGlyph();

    void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

protected:

    virtual tlp::Coord getAnchor(const tlp::Coord &vector) const;

};

#endif // DIAMONDGLYPH_H
