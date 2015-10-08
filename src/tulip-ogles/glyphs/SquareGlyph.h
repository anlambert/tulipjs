#ifndef RECTANGLEGLYPH_H
#define RECTANGLEGLYPH_H

#include "Glyph.h"

class SquareGlyph : public Glyph {

public:

    SquareGlyph();

protected:

    virtual tlp::Coord getAnchor(const tlp::Coord &vector) const;

};

#endif // RECTANGLEGLYPH_H
