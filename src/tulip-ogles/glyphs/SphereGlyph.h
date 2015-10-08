#ifndef SPHEREGLYPH_H
#define SPHEREGLYPH_H

#include "Glyph.h"

class SphereGlyph : public Glyph {

public:

    SphereGlyph();

    void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

    void getTextBoundingBox(tlp::BoundingBox &boundingBox);

    bool glyph2D() const {
        return false;
    }

    const std::vector<tlp::Coord> &getGlyphNormals() {
        return _vertices;
    }

private :

    void generateSphereData(int space);
};

#endif // SPHEREGLYPH_H
