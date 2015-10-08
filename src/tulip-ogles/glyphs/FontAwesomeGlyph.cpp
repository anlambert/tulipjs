#include <GL/glew.h>

#include "FontAwesomeGlyph.h"

#include "FTVectoriser.h"

#include "FTLibrary.h"

#include <tulip/TulipFontAwesome.h>
#include <tulip/BoundingBox.h>
#include <tulip/TlpTools.h>

#include <map>

#define HRES  64
#define HRESf 64.f
#define DPI   72

FontAwesomeGlyph::FontAwesomeGlyph(unsigned int iconCodePoint) {

  const FT_Library* library = FTLibrary::instance().getLibrary();

  FT_Error err;

  FT_Face face;

  /* Load face */

  err = FT_New_Face(*library, "resources/fontawesome-webfont.ttf", 0, &face);

  if (err) {
    FTLibrary::instance().printError(err);
    return;
  }

  /* Select charmap */
  err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);

  if (err) {
    FTLibrary::instance().printError(err);
    return;
  }

  float size = 20;

  /* Set char size */
  err = FT_Set_Char_Size(face, static_cast<int>(size * HRES), 0, DPI * HRES, DPI * HRES);

  if (err) {
    FTLibrary::instance().printError(err);
    return;
  }

  FT_UInt glyph_index = FT_Get_Char_Index(face, iconCodePoint);

  if (err) {
    FTLibrary::instance().printError(err);
    return;
  }

  err = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING);

  if (err) {
    FTLibrary::instance().printError(err);
    return;
  }

  FTVectoriser vectoriser(face->glyph);

  vectoriser.MakeMesh(1.0, 1, 0.0);

  const FTMesh *mesh = vectoriser.GetMesh();

  tlp::BoundingBox meshBB;

  std::map<tlp::Coord, unsigned int> vertexIdx;

  unsigned int idx = 0;
  for(unsigned int t = 0; t < mesh->TesselationCount(); ++t) {
    const FTTesselation* subMesh = mesh->Tesselation(t);
    for (unsigned int i = 0; i < subMesh->PointCount(); ++i) {
      FTPoint point = subMesh->Point(i);
      tlp::Coord p(point.Xf() / HRESf, point.Yf() / HRESf, 0.0f);
      if (vertexIdx.find(p) == vertexIdx.end()) {
        meshBB.expand(p);
        _vertices.push_back(p);
        _indices.push_back(idx++);
        vertexIdx[_vertices.back()] = _indices.back();
      } else {
        _indices.push_back(vertexIdx[p]);
      }
    }
  }

  for (unsigned int t = 0 ; t < vectoriser.ContourCount() ; ++t)  {
    const FTContour* contour = vectoriser.Contour(t);
    for (unsigned int i = 0 ; i < contour->PointCount() ; ++i) {
      FTPoint point = contour->Point(i);
      tlp::Coord p(point.Xf() / HRESf, point.Yf() / HRESf, 0.0f);
      FTPoint point2;
      if (i < contour->PointCount() - 1) {
        point2 = contour->Point(i+1);
      } else {
        point2 = contour->Point(0);
      }
      tlp::Coord p2(point2.Xf() / HRESf, point2.Yf() / HRESf, 0.0f);
      _outlineIndices.push_back(vertexIdx[p]);
      _outlineIndices.push_back(vertexIdx[p2]);
    }
  }


  tlp::Coord minC = meshBB[0];
  tlp::Coord maxC = meshBB[1];

  for (size_t i = 0 ; i < _vertices.size() ; ++i) {
    if (meshBB.height() > meshBB.width()) {
      _vertices[i][0] = ((_vertices[i][0] - minC[0]) / (maxC[0]-minC[0]) - 0.5) * (meshBB.width() / float(meshBB.height()));
      _vertices[i][1] = ((_vertices[i][1] - minC[1]) / (maxC[1]-minC[1])) - 0.5;
    } else {
      _vertices[i][0] = ((_vertices[i][0] - minC[0]) / (maxC[0]-minC[0])) - 0.5;
      _vertices[i][1] = (((_vertices[i][1] - minC[1]) / (maxC[1]-minC[1])) - 0.5) * (meshBB.height() / float(meshBB.width()));
    }

  }

}
