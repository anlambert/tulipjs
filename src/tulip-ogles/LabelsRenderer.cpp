#include <GL/glew.h>

#include <sstream>
#include <fstream>
#include <set>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <tulip/IntegerProperty.h>

#include "GlShaderProgram.h"
#include "GlBuffer.h"
#include "LabelsRenderer.h"
#include "QuadTree.h"
#include "Utils.h"
#include "utf8.h"
#include "GlRect2D.h"
#include "GlConcavePolygon.h"
#include "glyphs/GlyphsManager.h"

using namespace std;
using namespace tlp;

static Vec5f makeVec5f(float v1, float v2, float v3, float v4, float v5) {
  Vec5f ret;
  ret[0] = v1;
  ret[1] = v2;
  ret[2] = v3;
  ret[3] = v4;
  ret[4] = v5;
  return ret;
}

static string vertexShaderSrc =
#ifdef __EMSCRIPTEN__
    "precision highp float;\n"
    "precision highp int;\n"
#else
    "#version 120\n"
#endif

    "uniform mat4 u_projectionMatrix;"
    "uniform mat4 u_modelviewMatrix;"
    "uniform vec3 u_textBBMin;"
    "uniform vec3 u_textBBMax;"
    "uniform bool u_billboarding;"

    "attribute vec3 a_position;"
    "attribute vec2 a_texCoord;"

    "varying vec2 v_texCoord;"

    "mat4 scaleMatrix(vec3 scale) {"
    "  mat4 ret = mat4(1.0);"
    "  ret[0][0] = scale[0];"
    "  ret[1][1] = scale[1];"
    "  ret[2][2] = scale[2];"
    "  return ret;"
    "}"

    "mat4 translationMatrix(vec3 center) {"
    "  mat4 ret = mat4(1.0);"
    "  ret[3][0] = center[0];"
    "  ret[3][1] = center[1];"
    "  ret[3][2] = center[2];"
    "  return ret;"
    "}"

    "mat4 billboardMatrix(mat4 originalMdvMatrix, vec3 size) {"
    "  mat4 ret = originalMdvMatrix;"
    "  ret[0][0] = size.x;"
    "  ret[1][1] = size.y;"
    "  ret[2][2] = size.z;"
    "  ret[0][1] = 0.0;"
    "  ret[0][2] = 0.0;"
    "  ret[1][0] = 0.0;"
    "  ret[1][2] = 0.0;"
    "  ret[2][0] = 0.0;"
    "  ret[2][1] = 0.0;"
    "  return ret;"
    "}"

    "void main(void) {"
    "  v_texCoord = a_texCoord;"
    "  vec3 center = (u_textBBMin+u_textBBMax)/2.0;"
    "  float textBBWidth = u_textBBMax[0] - u_textBBMin[0];"
    "  float textBBHeight = u_textBBMax[1] - u_textBBMin[1];"
    "  mat4 mdv = u_modelviewMatrix * translationMatrix(center) * scaleMatrix(vec3(textBBWidth, textBBHeight, 1.0));"
    "  if (u_billboarding) {"
    "    mdv = billboardMatrix(mdv, vec3(textBBWidth, textBBHeight, 1.0));"
    "  }"
    "  gl_Position = u_projectionMatrix * mdv * vec4(a_position, 1.0);"
    "}"
    ;

static string fragmentShaderSrc =
#ifdef __EMSCRIPTEN__
    "#extension GL_OES_standard_derivatives : enable\n"
    "precision highp float;\n"
    "precision highp int;\n"
#else
    "#version 120\n"
    "#extension GL_OES_standard_derivatives : enable\n"
#endif

    "uniform sampler2D u_texture;"
    "uniform sampler2D u_textureDF;"
    "uniform bool u_useDistanceField;"
    "uniform vec4 u_color;"

    "varying vec2 v_texCoord;"

    "const float glyph_center = 0.5;"

    "void main(void) {"
    "  float alpha = 0.0;"
    "  if (u_useDistanceField) {"
    "    float dist  = texture2D(u_textureDF, v_texCoord).a;"
    "    float width = fwidth(dist);"
    "    alpha = smoothstep(glyph_center-width, glyph_center+width, dist);"
    "  } else {"
    "    alpha  = texture2D(u_texture, v_texCoord).a;"
    "  }"
    "  gl_FragColor = vec4(u_color.rgb, u_color.a * alpha);"
    "}"
    ;

static wstring stringToWString(const string &s) {
  string tmp = s + " ";
  string cleanUtf8Str;
  utf8::replace_invalid(tmp.begin(), tmp.end(), back_inserter(cleanUtf8Str));
  wstring ret;
  utf8::utf8to32(cleanUtf8Str.begin(), cleanUtf8Str.end()-1, back_inserter(ret));
  return ret;
}

static vector<string> splitStringToLines(const string &text) {
  vector<string> textVector;
  size_t lastPos=0;
  size_t pos=text.find_first_of("\n");

  while(pos!=string::npos) {
    textVector.push_back(text.substr(lastPos,pos-lastPos));
    lastPos=pos+1;
    pos=text.find_first_of("\n",pos+1);
  }
  textVector.push_back(text.substr(lastPos));
  return textVector;
}


#ifdef __EMSCRIPTEN__

static set<string> fontFileRequested;

static void fontFileLoaded(const char *fontFile) {
  cout << "Font file " << fontFile << " successfully loaded" << endl;
}

static void fontFileLoadError(const char *fontFile) {
  cout << "Error when trying to load font file " << fontFile << endl;
}
#endif

map<string, LabelsRenderer *> LabelsRenderer::_instances;
string LabelsRenderer::_currentCanvasId("");

LabelsRenderer* LabelsRenderer::instance() {
  return instance(_currentCanvasId);
}

LabelsRenderer *LabelsRenderer::instance(const std::string &canvasId) {
    if (_instances.find(canvasId) == _instances.end()) {
        _instances[canvasId] = new LabelsRenderer();
    }
    return _instances[canvasId];
}

LabelsRenderer::LabelsRenderer() :
  _textureFont(NULL), _textureAtlas(NULL), _textureAtlasDF(0),
  _labelsScaled(false),
  _minSize(12), _maxSize(72), _occlusionTest(true) {

  _labelsShader = new GlShaderProgram();
  _labelsShader->addShaderFromSourceCode(GlShader::Vertex, vertexShaderSrc);
  _labelsShader->addShaderFromSourceCode(GlShader::Fragment, fragmentShaderSrc);
  _labelsShader->link();

  if (!_labelsShader->isLinked()) {
    _labelsShader->printInfoLog();
  }

  loadFontFromFile("resources/DejaVuSans.ttf");

}

LabelsRenderer::~LabelsRenderer() {
  delete _textureFont;
  delete _textureAtlas;
  delete _labelsShader;
}

const int nbFloatPerVertex = 5;
const size_t fontPointSize = 18;
const size_t fontPointSizeDF = fontPointSize * 2;

void LabelsRenderer::initFont() {
  if (fontInit() || _fontFile.empty()) return;
  if (fileExists(_fontFile.c_str())) {
    _textureAtlas = new TextureAtlas(1024, 1024, 1);
    _textureFont = new TextureFont(_textureAtlas, fontPointSize, _fontFile);
    _textureFont->setOutlineType(2);
    _textureFont->setOutlineThickness(0.5);

    _textureAtlasDF = new TextureAtlas(1024, 1024, 1, true);
    _textureFontDF = new TextureFont(_textureAtlasDF, fontPointSizeDF, _fontFile);
    _textureFontDF->setOutlineType(2);
    _textureFontDF->setOutlineThickness(0.5);

    const wchar_t* cache = L" !\"#$%&'()*+,-./0123456789:;<=>?"
                           L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                           L"`abcdefghijklmnopqrstuvwxyz{|}~"
                           L"éèàù";

    _textureFont->loadGlyphs(cache);
    _textureFontDF->loadGlyphs(cache);

  } else {
    std::cout << _fontFile << " is not loaded" << std::endl;
  }
}

void LabelsRenderer::loadFontFromFile(const string &fontFile) {
  if (fontInit()) return;
  _fontFile = fontFile;
#ifdef __EMSCRIPTEN__
  if (fileExists(fontFile.c_str())) {
    initFont();
  } else if (fontFileRequested.find(fontFile) == fontFileRequested.end()) {
    fontFileRequested.insert(fontFile);
    emscripten_async_wget(_fontFile.c_str(), _fontFile.c_str(), fontFileLoaded, fontFileLoadError);
  }
#else
  initFont();
#endif
}

bool LabelsRenderer::fontInit() const {
  return _textureFont != NULL;
}

void LabelsRenderer::addOrUpdateNodeLabel(Graph *graph, node n) {
  if (!fontInit()) return;
  StringProperty *viewLabel = graph->getProperty<StringProperty>("viewLabel");
  const string &label = viewLabel->getNodeValue(n);
  if (!label.empty()) {
    _nodeLabelAspectRatio[graph][n] = getTextAspectRatio(_textureFont, label);
    _nodeLabelAspectRatioDF[graph][n] = getTextAspectRatio(_textureFontDF, label);
    _nodeLabelNbLines[graph][n] = std::count(label.begin(), label.end(), '\n')+1;
  }
}

void LabelsRenderer::removeNodeLabel(tlp::Graph *graph, tlp::node n) {
  _nodeLabelAspectRatio[graph].erase(n);
  _nodeLabelAspectRatioDF[graph].erase(n);
  _nodeLabelNbLines[graph].erase(n);
}

Vec2f computeScreenPos(const MatrixGL &transformMatrix, const Vec4i &viewport, const Vec3f &point) {
  Vec4f screenCoord = Vec4f(point, 1.0) * transformMatrix;
  screenCoord /= screenCoord[3];
  screenCoord[0] = (screenCoord[0]+1.)*viewport[2]/2. + viewport[0];
  screenCoord[1] = (screenCoord[1]+1.)*viewport[3]/2. + viewport[1];
  return Vec2f(screenCoord[0], screenCoord[1]);
}

void LabelsRenderer::getLabelRenderingData(TextureFont *textureFont, const string &text, std::pair<std::vector<Vec5f>, std::vector<unsigned short> > &renderingDataRet) {
  vector<Vec5f> renderingData;
  vector<unsigned short> indices;
  renderingData.reserve(text.size()*4);
  indices.reserve(text.size()*6);

  if (!text.empty()) {

    vector<string> textVector = splitStringToLines(text);

    BoundingBox textBB;

    float bltbl = textureFont->ascender() - textureFont->descender() + textureFont->linegap();
    float penY = 0;

    vector<pair<unsigned int, float> > linesData;
    for (size_t j = 0 ; j < textVector.size() ; ++j) {
      wstring str = stringToWString(textVector[j]);
      BoundingBox lineBB;
      getLineRenderingDataUnscaled(textureFont, str, renderingData, indices, lineBB, penY);
      penY -= bltbl;
      textBB.expand(lineBB[0]);
      textBB.expand(lineBB[1]);
      linesData.push_back(make_pair(str.size()*4, lineBB.width()));
    }

    Vec2f minV(renderingData[0][0],renderingData[0][1]);
    Vec2f maxV(renderingData[0][0],renderingData[0][1]);

    for (size_t i = 1; i < renderingData.size(); ++i) {
      minV[0] = min(renderingData[i][0], minV[0]);
      minV[1] = min(renderingData[i][1], minV[1]);
      maxV[0] = max(renderingData[i][0], maxV[0]);
      maxV[1] = max(renderingData[i][1], maxV[1]);
    }

    Vec2f center = (minV + maxV) / 2.f;
    Vec2f scale  = (maxV - minV);

    unsigned int k = 0;
    for (size_t i = 0 ; i < linesData.size() ; ++i) {
      for (size_t j = 0; j < linesData[i].first ; ++j) {
        renderingData[k][0] += (textBB.width() - linesData[i].second)/2;
        renderingData[k][0] = (renderingData[k][0] - center[0]) / scale[0];
        renderingData[k][1] = (renderingData[k][1] - center[1]) / scale[1];
        renderingData[k][2] = 0.0;

        ++k;
      }
    }
  }

  renderingDataRet.first = renderingData;
  renderingDataRet.second = indices;
}

void LabelsRenderer::getLineRenderingDataUnscaled(TextureFont *textureFont, const wstring &line, vector<Vec5f> &renderingData, vector<unsigned short> &indices, BoundingBox &lineBB, float penY) {

  float width = 0.f, ymin = 0.f, ymax = 0.f;

  vec2 pen(0 ,penY);
  for (size_t i = 0 ; i < line.size() ; ++i) {
    TextureGlyph *glyph = textureFont->getGlyph(line[i]);
    if (glyph != NULL) {

      float kerning = 0;
      if (i > 0) {
        kerning = glyph->getKerning(line[i-1]);
      }
      pen.x += kerning;
      float x0 = pen.x + glyph->offset_x;
      float y0 = pen.y + glyph->offset_y;
      float x1 = x0 + glyph->width;
      float y1 = y0 - glyph->height;

      float s0 = glyph->s0;
      float t0 = glyph->t0;
      float s1 = glyph->s1;
      float t1 = glyph->t1;

      unsigned short startIdx = static_cast<unsigned short>(renderingData.size());

      renderingData.push_back(makeVec5f(x0, y0, 0, s0, t0));
      renderingData.push_back(makeVec5f(x1, y1, 0, s1, t1));
      renderingData.push_back(makeVec5f(x0, y1, 0, s0, t1));
      renderingData.push_back(makeVec5f(x1, y0, 0, s1, t0));

      indices.push_back(startIdx);
      indices.push_back(startIdx+1);
      indices.push_back(startIdx+2);
      indices.push_back(startIdx);
      indices.push_back(startIdx+3);
      indices.push_back(startIdx+1);

      pen.x += glyph->advance_x;

      width += kerning;
      width += glyph->advance_x;
      float miny = -(static_cast<int>(glyph->height) - glyph->offset_y);
      float maxy = miny + glyph->height;
      ymin = miny < ymin ? miny : ymin;
      ymax = maxy > ymax ? maxy : ymax;
    }
  }

  lineBB[0] = Vec3f(0, penY);
  lineBB[1] = Vec3f(width, penY + (ymax - ymin));

}

tlp::BoundingBox LabelsRenderer::getLabelRenderingBoxScaled(const tlp::BoundingBox &renderingBox, float aspectRatio) {
  BoundingBox renderingBoxScaled;

  float scaleX = renderingBox.width();
  float scaleY = scaleX / aspectRatio;

  if (scaleY > renderingBox.height()) {
    aspectRatio = 1.f / aspectRatio;
    scaleY = renderingBox.height();
    scaleX = scaleY / aspectRatio;
  }

  float z = renderingBox[1][2];

  renderingBoxScaled[0] = Coord(renderingBox.center()[0] - scaleX / 2.f, renderingBox.center()[1] - scaleY / 2.f, z);
  renderingBoxScaled[1] = Coord(renderingBox.center()[0] + scaleX / 2.f, renderingBox.center()[1] + scaleY / 2.f, z);

  return renderingBoxScaled;
}

float LabelsRenderer::getTextAspectRatio(TextureFont *textureFont, const string &text) {

  vector<string> textVector = splitStringToLines(text);

  float y = 0;
  float bltbl = textureFont->ascender() - textureFont->descender() + textureFont->linegap();

  tlp::BoundingBox textBB;

  for (size_t i = 0 ; i < textVector.size() ; ++i) {

    float width = 0.f, ymin = 0.f, ymax = 0.f;

    wstring textW = stringToWString(textVector[i]);
    textureFont->loadGlyphs(textW.c_str());

    for (size_t j = 0 ; j < textW.size() ; ++j) {
      TextureGlyph *glyph = textureFont->getGlyph(textW[j]);
      if( glyph != NULL ) {
        if (j > 0) {
          float kerning = 0;
          kerning = glyph->getKerning(textW[j-1]);
          width += kerning;
        }
        width += glyph->advance_x;
        float miny = -(static_cast<int>(glyph->height) - glyph->offset_y);
        float maxy = miny + glyph->height;
        ymin = miny < ymin ? miny : ymin;
        ymax = maxy > ymax ? maxy : ymax;

      } else {
        cerr << "Font error : no glyph for char : " << text[i] << endl;
      }
    }
    textBB.expand(Coord(0, y));
    textBB.expand(Coord(width, y + (ymax-ymin)));

    y -= bltbl;

  }

  return textBB.width() / textBB.height();
}

void LabelsRenderer::renderOneLabel(const Camera &camera, const string &text, const BoundingBox &renderingBox, const Color &labelColor) {;
  initFont();
  if (!fontInit() || text.empty()) return;

  pair<vector<Vec5f>, vector<unsigned short> > labelData;

  BoundingBox textBBScaled = getLabelRenderingBoxScaled(renderingBox, getTextAspectRatio(_textureFont, text));

  Vec4i viewport = camera.getViewport();

  Vec2f textBBMinScr = computeScreenPos(camera.projectionMatrix(), viewport, textBBScaled[0]);
  Vec2f textBBMaxScr = computeScreenPos(camera.projectionMatrix(), viewport, textBBScaled[1]);

  float screenH = (textBBMaxScr[1] - textBBMinScr[1]) / std::count(text.begin(), text.end(), '\n')+1;;

  _labelsShader->activate();
  _labelsShader->setUniformMat4Float("u_projectionMatrix", camera.projectionMatrix());
  _labelsShader->setUniformMat4Float("u_modelviewMatrix", camera.modelviewMatrix());
  _labelsShader->setUniformBool("u_billboarding", false);
  _labelsShader->setUniformTextureSampler("u_texture", 0);
  _labelsShader->setUniformTextureSampler("u_textureDF", 1);

  if (screenH < fontPointSize) {
    getLabelRenderingData(_textureFont, text, labelData);
    _labelsShader->setUniformBool("u_useDistanceField", false);
    _textureAtlas->bind();
  } else {
    getLabelRenderingData(_textureFontDF, text, labelData);
    textBBScaled = getLabelRenderingBoxScaled(renderingBox, getTextAspectRatio(_textureFontDF, text));
    _labelsShader->setUniformBool("u_useDistanceField", true);
    _textureAtlasDF->bind();
  }

  _labelsShader->setUniformVec3Float("u_textBBMin", textBBScaled[0]);
  _labelsShader->setUniformVec3Float("u_textBBMax", textBBScaled[1]);
  _labelsShader->setUniformColor("u_color", labelColor);

  glActiveTexture(GL_TEXTURE1);
  _textureAtlasDF->bind();
  glActiveTexture(GL_TEXTURE0);
  _textureAtlas->bind();

  GlBuffer labelDataBuffer(GlBuffer::VertexBuffer);
  labelDataBuffer.bind();
  labelDataBuffer.allocate(labelData.first);

  _labelsShader->setVertexAttribPointer("a_position", 3, GL_FLOAT, GL_FALSE, nbFloatPerVertex * sizeof(float), BUFFER_OFFSET(0));
  _labelsShader->setVertexAttribPointer("a_texCoord", 2, GL_FLOAT, GL_FALSE, nbFloatPerVertex * sizeof(float), BUFFER_OFFSET(3*sizeof(float)));

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GlBuffer labelIndicesBuffer(GlBuffer::IndexBuffer);
  labelIndicesBuffer.bind();
  labelIndicesBuffer.allocate(labelData.second);

  glDrawElements(GL_TRIANGLES, labelData.second.size(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));

  glDisable(GL_BLEND);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  labelDataBuffer.release();
  labelIndicesBuffer.release();
  _labelsShader->desactivate();
}

static BoundingBox labelBoundingBoxForNode(Graph *graph, node n) {
  IntegerProperty *viewShape = graph->getProperty<IntegerProperty>("viewShape");
  LayoutProperty *viewLayout = graph->getProperty<LayoutProperty>("viewLayout");
  SizeProperty *viewSize = graph->getProperty<SizeProperty>("viewSize");
  BoundingBox renderingBox;
  GlyphsManager::getGlyph(viewShape->getNodeValue(n))->getTextBoundingBox(renderingBox);
  const Coord &pos = viewLayout->getNodeValue(n);
  const Size &size = viewSize->getNodeValue(n) * Size(renderingBox.width(), renderingBox.height(), renderingBox.depth());
  return BoundingBox(pos - size/2.f, pos + size/2.f);
}

static void adjustTextBoundingBox(BoundingBox &textBB, const Camera &camera, float minSize, float maxSize, unsigned int nbLines) {
  Vec4i viewport = camera.getViewport();
  Vec2f textBBMinScr = computeScreenPos(camera.projectionMatrix(), viewport, textBB[0]);
  Vec2f textBBMaxScr = computeScreenPos(camera.projectionMatrix(), viewport, textBB[1]);

  float screenH = (textBBMaxScr[1] - textBBMinScr[1]) / nbLines;

  Vec3f center = textBB.center();
  Vec3f scale = textBB[1] - textBB[0];
  float ratio = textBB.width() / textBB.height();
  float scaleX = textBB.width();

  if (screenH < minSize) {
    scaleX *= (minSize/screenH);
  }
  if (screenH > maxSize) {
    scaleX *= (maxSize/screenH);
  }

  float scaleY = scaleX/ratio;

  textBB[0][0] = ((textBB[0][0] - center[0]) / scale[0]) * scaleX + center[0];
  textBB[0][1] = ((textBB[0][1] - center[1]) / scale[1]) * scaleY + center[1];
  textBB[1][0] = ((textBB[1][0] - center[0]) / scale[0]) * scaleX + center[0];
  textBB[1][1] = ((textBB[1][1] - center[1]) / scale[1]) * scaleY + center[1];
}

void LabelsRenderer::renderGraphNodesLabels(Graph *graph, const Camera &camera, const Color &selectionColor, bool billboarded) {

  if (!fontInit()) return;

  BooleanProperty *viewSelection = graph->getProperty<BooleanProperty>("viewSelection");
  ColorProperty *viewLabelColor = graph->getProperty<ColorProperty>("viewLabelColor");
  StringProperty *viewLabel = graph->getProperty<StringProperty>("viewLabel");

  Vec4i viewport = camera.getViewport();

  vector<Vec5f> renderingData;
  vector<BoundingBox> renderingBox;
  vector<float> labelScreenH;
  vector<Color> labelColor;
  vector<unsigned short> indices;
  vector<unsigned int> renderingDataOffset;
  vector<pair<unsigned int, unsigned int> > indicesCountAndOffset;

  vector<vector<Vec2f> > renderedLabelsScrRect;
  vector<BoundingBox> renderedLabelsScrBB;

  for (vector<node>::iterator it = _labelsToRender[graph].begin() ; it != _labelsToRender[graph].end() ; ++it) {

    if (_nodeLabelAspectRatio[graph].find(*it) == _nodeLabelAspectRatio[graph].end()) {
        continue;
    }

    BoundingBox nodeBB = labelBoundingBoxForNode(graph, *it);
    BoundingBox textBB = getLabelRenderingBoxScaled(nodeBB, _nodeLabelAspectRatio[graph][*it]);

    if (!_labelsScaled) {

      adjustTextBoundingBox(textBB, camera, _minSize, _maxSize, _nodeLabelNbLines[graph][*it]);
    }

    bool canRender = true;

    if (_occlusionTest) {

      if (billboarded || !camera.hasRotation()) {
        BoundingBox textScrBB;
        textScrBB.expand(Vec3f(computeScreenPos(camera.transformMatrixBillboard(), viewport, textBB[0]), 0));
        textScrBB.expand(Vec3f(computeScreenPos(camera.transformMatrixBillboard(), viewport, textBB[1]), 0));

        for (size_t i = 0 ; i < renderedLabelsScrBB.size() ; ++i) {
          if (textScrBB.intersect(renderedLabelsScrBB[i])) {
            canRender = false;
            break;
          }
        }

        if (canRender) {
          renderedLabelsScrBB.push_back(textScrBB);
        }

      } else {
        vector<Vec2f> textScrRect;
        textScrRect.push_back(computeScreenPos(camera.transformMatrix(), viewport, textBB[0]));
        textScrRect.push_back(computeScreenPos(camera.transformMatrix(), viewport, Vec3f(textBB[0][0]+textBB.width(), textBB[0][1], textBB[0][2])));
        textScrRect.push_back(computeScreenPos(camera.transformMatrix(), viewport, textBB[1]));
        textScrRect.push_back(computeScreenPos(camera.transformMatrix(), viewport, Vec3f(textBB[0][0], textBB[0][1]+textBB.height(), textBB[0][2])));

        for (size_t i = 0 ; i < renderedLabelsScrRect.size() ; ++i) {
          if (convexPolygonsIntersect(textScrRect, renderedLabelsScrRect[i])) {
            canRender = false;
            break;
          }
        }

        if (canRender) {
          renderedLabelsScrRect.push_back(textScrRect);
        }
      }
    }

    if (canRender) {

      Vec2f textBBMinScr = computeScreenPos(camera.projectionMatrix(), viewport, textBB[0]);
      Vec2f textBBMaxScr = computeScreenPos(camera.projectionMatrix(), viewport, textBB[1]);

      float screenH = (textBBMaxScr[1] - textBBMinScr[1]) / _nodeLabelNbLines[graph][*it];

      pair<vector<Vec5f>, vector<unsigned short> > labelData;
      if (screenH < fontPointSize) {
        getLabelRenderingData(_textureFont, viewLabel->getNodeValue(*it), labelData);
      } else {
        textBB = getLabelRenderingBoxScaled(nodeBB, _nodeLabelAspectRatioDF[graph][*it]);
        adjustTextBoundingBox(textBB, camera, _minSize, _maxSize, _nodeLabelNbLines[graph][*it]);
        getLabelRenderingData(_textureFontDF, viewLabel->getNodeValue(*it), labelData);
      }

      renderingDataOffset.push_back(renderingData.size()*nbFloatPerVertex);
      indicesCountAndOffset.push_back(make_pair(labelData.second.size(), indices.size()));
      renderingData.insert(renderingData.end(), labelData.first.begin(), labelData.first.end());
      indices.insert(indices.end(), labelData.second.begin(), labelData.second.end());

      if (!viewSelection->getNodeValue(*it)) {
        labelColor.push_back(viewLabelColor->getNodeValue(*it));
      } else {
        labelColor.push_back(selectionColor);
      }

      renderingBox.push_back(textBB);
      labelScreenH.push_back(screenH);

    }

  }

  if (renderingData.empty()) return;

  GlBuffer labelsDataBuffer(GlBuffer::VertexBuffer);
  labelsDataBuffer.bind();
  labelsDataBuffer.allocate(renderingData);

  GlBuffer labelsIndicesBuffer(GlBuffer::IndexBuffer);
  labelsIndicesBuffer.bind();
  labelsIndicesBuffer.allocate(indices);

  glActiveTexture(GL_TEXTURE1);
  _textureAtlasDF->bind();

  glActiveTexture(GL_TEXTURE0);
  _textureAtlas->bind();

  _labelsShader->activate();
  _labelsShader->setUniformMat4Float("u_projectionMatrix", camera.projectionMatrix());
  _labelsShader->setUniformMat4Float("u_modelviewMatrix", camera.modelviewMatrix());
  _labelsShader->setUniformTextureSampler("u_texture", 0);
  _labelsShader->setUniformTextureSampler("u_textureDF", 1);
  _labelsShader->setUniformBool("u_billboarding", billboarded);

  for (size_t i = 0 ; i < renderingDataOffset.size() ; ++i) {

    _labelsShader->setUniformVec3Float("u_textBBMin", renderingBox[i][0]);
    _labelsShader->setUniformVec3Float("u_textBBMax", renderingBox[i][1]);
    _labelsShader->setUniformColor("u_color", labelColor[i]);
    _labelsShader->setUniformBool("u_useDistanceField", labelScreenH[i] >= fontPointSize);

    _labelsShader->setVertexAttribPointer("a_position", 3, GL_FLOAT, GL_FALSE, nbFloatPerVertex * sizeof(float), BUFFER_OFFSET(renderingDataOffset[i]*sizeof(float)));
    _labelsShader->setVertexAttribPointer("a_texCoord", 2, GL_FLOAT, GL_FALSE, nbFloatPerVertex * sizeof(float), BUFFER_OFFSET((renderingDataOffset[i]+3)*sizeof(float)));

    glDrawElements(GL_TRIANGLES, indicesCountAndOffset[i].first, GL_UNSIGNED_SHORT, BUFFER_OFFSET(indicesCountAndOffset[i].second*sizeof(unsigned short)));

  }

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void LabelsRenderer::clearGraphNodesLabelsRenderingData(Graph *graph) {
  if (!graph) return;
  _nodeLabelNbLines.erase(graph);
  _nodeLabelAspectRatio.erase(graph);
}


