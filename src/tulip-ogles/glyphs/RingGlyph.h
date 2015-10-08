#ifndef RINGGLYPH_H
#define RINGGLYPH_H

#include "Glyph.h"

class RingGlyph : public Glyph {

public:

  RingGlyph();

  virtual void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

};

#endif // RINGGLYPH_H
