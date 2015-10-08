#ifndef CYLINDERGLYPH_H
#define CYLINDERGLYPH_H

#include "Glyph.h"

class CylinderGlyph : public Glyph {

public:

  CylinderGlyph();

  bool glyph2D() const {
    return false;
  }

};

class HalfCylinderGlyph : public Glyph {

public:

  HalfCylinderGlyph();

  bool glyph2D() const {
    return false;
  }

};

#endif // CYLINDERGLYPH_H
