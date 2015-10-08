#ifndef CUBEOUTLINEDGLYPHTRANSPARENT_H
#define CUBEOUTLINEDGLYPHTRANSPARENT_H

#include "Glyph.h"

class CubeOutlinedGlyph;

class CubeOutlinedTransparentGlyph : public Glyph {

public:

  CubeOutlinedTransparentGlyph();

  ~CubeOutlinedTransparentGlyph();

  const std::vector<tlp::Coord> &getGlyphVertices() const;

  const std::vector<unsigned short> &getGlyphOutlineIndices() const;

  bool glyph2D() const {
      return false;
  }

protected:

  tlp::Coord getAnchor(const tlp::Coord &vector) const;

private:

  CubeOutlinedGlyph *_cubeOutlinedGlyph;

};

#endif // CUBEOUTLINEDGLYPHTRANSPARENT_H
