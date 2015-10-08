#ifndef NODEGLYPHS_H
#define NODEGLYPHS_H

#include "Glyph.h"

#include <map>
#include <string>

class GlyphsManager {

public:

  static const std::map<int, Glyph*> &getGlyphs() {
    return _glyphs;
  }

  static Glyph *getGlyph(int glyphId);


private:

  static std::map<int, Glyph*> _glyphs;

};

#endif // NODEGLYPHS_H
