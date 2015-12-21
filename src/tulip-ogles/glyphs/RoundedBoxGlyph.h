#ifndef ROUNDEDBOXGLYPH_H
#define ROUNDEDBOXGLYPH_H

#include "Glyph.h"

class RoundedBoxGlyph : public Glyph {

public:

  RoundedBoxGlyph();

  virtual void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

protected:

  tlp::Coord getAnchor(const tlp::Coord &vector) const;

};

#endif // ROUNDEDBOXGLYPH_H
