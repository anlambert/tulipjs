#ifndef CONEGLYPH_H
#define CONEGLYPH_H

#include "Glyph.h"

class ConeGlyph : public Glyph {

public:

    ConeGlyph(bool eeGlyph=false);

    void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

    bool glyph2D() const {
        return false;
    }

protected:

    virtual tlp::Coord getAnchor(const tlp::Coord &vector) const;

    bool _eeGlyph;

};

#endif // CONEGLYPH_H
