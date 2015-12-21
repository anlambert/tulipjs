#include <tulip/TulipViewSettings.h>
#include <tulip/TulipFontAwesome.h>

#include "GlyphsManager.h"
#include "SquareGlyph.h"
#include "TriangleGlyph.h"
#include "CircleGlyph.h"
#include "PentagonGlyph.h"
#include "HexagonGlyph.h"
#include "RoundedBoxGlyph.h"
#include "CrossGlyph.h"
#include "DiamondGlyph.h"
#include "StarGlyph.h"
#include "SphereGlyph.h"
#include "CubeGlyph.h"
#include "CubeOutlinedGlyph.h"
#include "CubeOutlinedTransparentGlyph.h"
#include "RingGlyph.h"
#include "CylinderGlyph.h"
#include "ConeGlyph.h"
#include "FontAwesomeGlyph.h"
//#include "MeshGlyph.h"

using namespace std;

static Glyph* createGlyph(int glyphId) {

  if (glyphId == tlp::NodeShape::Square) {
    return new SquareGlyph();
  } else if (glyphId == tlp::NodeShape::Diamond) {
    return new DiamondGlyph();
  } else if (glyphId == tlp::NodeShape::Triangle) {
    return new TriangleGlyph();
  } else if (glyphId == tlp::NodeShape::Circle) {
    return new CircleGlyph();
  } else if (glyphId == tlp::NodeShape::Pentagon) {
    return new PentagonGlyph();
  } else if (glyphId == tlp::NodeShape::Hexagon) {
    return new HexagonGlyph();
  } else if (glyphId == tlp::NodeShape::RoundedBox) {
    return new RoundedBoxGlyph();
  } else if (glyphId == tlp::NodeShape::Cross) {
    return new CrossGlyph();
  } else if (glyphId == tlp::NodeShape::Star) {
    return new StarGlyph();
  } else if (glyphId == tlp::NodeShape::Sphere) {
    return new SphereGlyph();
  } else if (glyphId == tlp::NodeShape::Cube) {
    return new CubeGlyph();
  } else if (glyphId == tlp::NodeShape::CubeOutlined) {
    return new CubeOutlinedGlyph();
  } else if (glyphId == tlp::NodeShape::CubeOutlinedTransparent) {
    return new CubeOutlinedTransparentGlyph();
  } else if (glyphId == tlp::NodeShape::Ring) {
    return new RingGlyph();
  } else if (glyphId == tlp::NodeShape::Cylinder) {
    return new CylinderGlyph();
  } else if (glyphId == tlp::NodeShape::HalfCylinder) {
    return new HalfCylinderGlyph();
  } else if (glyphId == tlp::NodeShape::Cone) {
    return new ConeGlyph();
  } else if (glyphId == tlp::NodeShape::ChristmasTree) {
    return new FontAwesomeGlyph(tlp::TulipFontAwesome::getFontAwesomeIconCodePoint(tlp::TulipFontAwesome::Tree));
  } else if (glyphId >= 0xf000) {
    return new FontAwesomeGlyph(glyphId);
    //  } else if (glyphId == 100) {
    //    return new MeshGlyph();
  } else {
    return NULL;
  }
}


map<int, Glyph*> GlyphsManager::_glyphs;

Glyph *GlyphsManager::getGlyph(int glyphId) {

  if (glyphId == tlp::EdgeExtremityShape::Square) {
    glyphId = tlp::NodeShape::Square;
  }
  if (glyphId == tlp::EdgeExtremityShape::Diamond) {
    glyphId = tlp::NodeShape::Diamond;
  }
  if (glyphId == tlp::EdgeExtremityShape::Arrow) {
    glyphId = tlp::NodeShape::Triangle;
  }
  if (glyphId == tlp::EdgeExtremityShape::Circle) {
    glyphId = tlp::NodeShape::Circle;
  }
  if (glyphId == tlp::EdgeExtremityShape::Pentagon) {
    glyphId = tlp::NodeShape::Pentagon;
  }
  if (glyphId == tlp::EdgeExtremityShape::Hexagon) {
    glyphId = tlp::NodeShape::Hexagon;
  }
  if (glyphId == tlp::EdgeExtremityShape::Cross) {
    glyphId = tlp::NodeShape::Cross;
  }
  if (glyphId == tlp::EdgeExtremityShape::Star) {
    glyphId = tlp::NodeShape::Star;
  }
  if (glyphId == tlp::EdgeExtremityShape::Sphere) {
    glyphId = tlp::NodeShape::Sphere;
  }
  if (glyphId == tlp::EdgeExtremityShape::Cube) {
    glyphId = tlp::NodeShape::Cube;
  }
  if (glyphId == tlp::EdgeExtremityShape::CubeOutlinedTransparent) {
    glyphId = tlp::NodeShape::CubeOutlinedTransparent;
  }
  if (glyphId == tlp::EdgeExtremityShape::Ring) {
    glyphId = tlp::NodeShape::Ring;
  }
  if (glyphId == tlp::EdgeExtremityShape::Cylinder) {
    glyphId = tlp::NodeShape::Cylinder;
  }

  if (_glyphs.find(glyphId) == _glyphs.end()) {
    Glyph *glyph = createGlyph(glyphId);
    if (!glyph) {
      if (_glyphs.find(tlp::NodeShape::Square) == _glyphs.end()) {
        _glyphs[tlp::NodeShape::Square] = createGlyph(tlp::NodeShape::Square);
      }
      glyphId = tlp::NodeShape::Square;
    } else {
      _glyphs[glyphId] = glyph;
    }
  }
  return _glyphs[glyphId];
}
