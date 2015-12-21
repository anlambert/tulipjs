#ifndef STARGLYPH_H
#define STARGLYPH_H

#include "Glyph.h"

class StarGlyph : public Glyph {

public:

  StarGlyph();

  void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

};

#endif // STARGLYPH_H
