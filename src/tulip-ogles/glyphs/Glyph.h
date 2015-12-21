#ifndef NODEGLYPH_H
#define NODEGLYPH_H

#include <tulip/Size.h>
#include <tulip/Coord.h>
#include <tulip/BoundingBox.h>

#include <vector>

class Glyph {
public:

  Glyph() {}
  virtual ~Glyph() {}

  virtual void getIncludeBoundingBox(tlp::BoundingBox &boundingBox);

  virtual void getTextBoundingBox(tlp::BoundingBox &boundingBox) {
    getIncludeBoundingBox(boundingBox);
  }

  virtual const std::vector<tlp::Coord> &getGlyphVertices() const {
    return _vertices;
  }

  virtual const std::vector<tlp::Vec2f> &getGlyphTexCoords();

  virtual const std::vector<tlp::Coord> &getGlyphNormals();

  virtual const std::vector<unsigned short> &getGlyphVerticesIndices() const {
    return _indices;
  }

  virtual const std::vector<unsigned short> &getGlyphOutlineIndices() const {
    return _outlineIndices;
  }

  virtual bool glyph2D() const {
    return true;
  }

  /*
   * return a point where an edge coming from "from" can be attached
   * by default, the point will be on the surface of the largest sphere contained
   * inside the unit cube (before scaling).
   */
  virtual tlp::Coord getAnchor(const tlp::Coord &nodeCenter, const tlp::Coord &from,
                               const tlp::Size &scale, const double zRotation) const;


protected:

  /*
   * called by public method getAnchor to actually compute the anchor point
   * vector is coordinate of the point to anchor to, relative to nodecenter
   * glyph size is (1,1,1)
   * this is the method to redefine for each glyph where the default 'getAnchor' method
   * is inapropriated
   * Returned value is a vector to be applied to 'nodeCenter' in the public method
   */
  virtual tlp::Coord getAnchor(const tlp::Coord &vector) const;

  std::vector<tlp::Coord> _vertices;
  std::vector<unsigned short> _indices;
  std::vector<unsigned short> _outlineIndices;
  std::vector<tlp::Vec2f> _texCoords;
  std::vector<tlp::Coord> _normals;

};

#endif // NODEGLYPH_H

