#ifndef CUBEOUTLINEDGLYPH_H
#define CUBEOUTLINEDGLYPH_H

#include "Glyph.h"

class CubeGlyph;

class CubeOutlinedGlyph : public Glyph {

  friend class CubeOutlinedTransparentGlyph;

public:

  CubeOutlinedGlyph();
  ~CubeOutlinedGlyph();

  void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

  void getTextBoundingBox(tlp::BoundingBox &boundingBox);

  const std::vector<tlp::Coord> &getGlyphVertices() const;

  const std::vector<tlp::Vec2f> &getGlyphTexCoords();

  const std::vector<tlp::Coord> &getGlyphNormals();

  const std::vector<unsigned short> &getGlyphVerticesIndices() const;

  bool glyph2D() const {
    return false;
  }

protected:

  tlp::Coord getAnchor(const tlp::Coord &vector) const;

private:

  CubeGlyph *_cubeGlyph;

};

#endif // CUBEOUTLINEDGLYPH_H
