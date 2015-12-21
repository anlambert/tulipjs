#ifndef TRIANGLEGLYPH_H
#define TRIANGLEGLYPH_H

#include "Glyph.h"

class TriangleGlyph : public Glyph {

public:

  TriangleGlyph();

  virtual void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

};

#endif // TRIANGLEGLYPH_H
