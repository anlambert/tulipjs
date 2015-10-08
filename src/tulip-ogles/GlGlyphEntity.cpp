#include "GlGlyphEntity.h"
#include "glyphs/GlyphsRenderer.h"

using namespace tlp;
using namespace std;

GlGlyphEntity::GlGlyphEntity(int glyphId) : _glyphId(glyphId), _center(Coord(0.f,0.f,0.f)), _size(1.f,1.f,1.f), _color(Color::Red),
                                            _texture(""), _borderWidth(0.f), _borderColor(Color::Black),
                                            _rotationAxisAndAngle(Vec4f(0.f,0.f,1.f,0.f)) {
  updateBoundingBox();
}

GlGlyphEntity::GlGlyphEntity(int glyphId, const Coord &center, const Size &size, const Color &color,
                             const string &texture, const float borderWidth, const Color &borderColor,
                             const Vec4f &rotationAxisAndAngle) : _glyphId(glyphId), _center(center), _size(size), _color(color),
                                                                  _texture(texture), _borderWidth(borderWidth), _borderColor(borderColor),
                                                                  _rotationAxisAndAngle(rotationAxisAndAngle) {
  updateBoundingBox();
}

void GlGlyphEntity::setCenter(const tlp::Coord &center) {
  _center = center;
  updateBoundingBox();
}

void GlGlyphEntity::setSize(const tlp::Size &size) {
  _size = size;
  updateBoundingBox();
}

void GlGlyphEntity::updateBoundingBox() {
  _boundingBox[0] = _center - _size / 2.f;
  _boundingBox[1] = _center + _size / 2.f;
}

void GlGlyphEntity::draw(const Camera &camera, const Light &light, bool pickingMode) {
  if (!pickingMode) {
    GlyphsRenderer::instance()->renderGlyph(camera, light, _glyphId, _center, _size, _color, _texture, _borderWidth, _borderColor, _rotationAxisAndAngle);
  } else {
    GlyphsRenderer::instance()->renderGlyph(camera, light, _glyphId, _center, _size, _pickingColor, _texture, _borderWidth, _pickingColor, _rotationAxisAndAngle, true);
  }

}

void GlGlyphEntity::draw(const Camera &camera, bool pickingMode) {
  GlEntity::draw(camera, pickingMode);
}
