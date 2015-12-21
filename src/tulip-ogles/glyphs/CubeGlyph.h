#ifndef CUBEGLYPH_H
#define CUBEGLYPH_H

#include "Glyph.h"

class CubeGlyph : public Glyph {

  friend class CubeOutlinedGlyph;

public:

  CubeGlyph();

  bool glyph2D() const {
    return false;
  }

protected:

  tlp::Coord getAnchor(const tlp::Coord &vector) const;
};

#endif // CUBEGLYPH_H
