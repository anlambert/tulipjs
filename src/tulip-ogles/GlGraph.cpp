#include <GL/glew.h>

#include <tulip/ForEach.h>
#include <tulip/LayoutProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/ColorProperty.h>
#include <tulip/DoubleProperty.h>
#include <tulip/StringProperty.h>
#include <tulip/IntegerProperty.h>
#include <tulip/BooleanProperty.h>
#include <tulip/TulipViewSettings.h>
#include <tulip/DrawingTools.h>
#include <tulip/TulipFontAwesome.h>

#include "GlScene.h"
#include "GlLayer.h"
#include "GlGraph.h"
#include "GlBuffer.h"
#include "GlShaderProgram.h"
#include "Utils.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "GlFrameBufferObject.h"
#include "glyphs/GlyphsManager.h"
#include "LabelsRenderer.h"
#include "GlQuadTreeLODCalculator.h"
#include "TimeMeasurer.h"
#include "Camera.h"
#include "Light.h"

#include "glyphs/GlyphsRenderer.h"


#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

using namespace std;
using namespace tlp;

static string curveVertexShaderSrc =
    "uniform mat4 u_projectionMatrix;"
    "uniform mat4 u_modelviewMatrix;"
    "uniform bool u_billboard;"
    "uniform bool u_textureActivated;"
    "uniform vec3 u_lookDir;"
    "uniform bool u_lineMode;"
    "uniform float u_stepKnots;"
    "uniform float u_curveInterpolationStep;"

    "const int MAX_CURVE_POINTS = 200;"

    "uniform vec4 u_curvePoints[MAX_CURVE_POINTS];"
    "uniform int u_nbCurvePoints;"
    "uniform float u_startSize;"
    "uniform float u_endSize;"
    "uniform vec4 u_startColor;"
    "uniform vec4 u_endColor;"
    "uniform vec3 u_startN;"
    "uniform vec3 u_endN;"

    "attribute vec3 a_position;"

    "varying vec4 v_color;"
    "varying vec2 v_texCoord;"

    "const float M_PI = 3.14159265358979323846;"

    "\n#ifdef BEZIER_VERTEX_SHADER\n"

    "vec3 computeCurvePoint(float t) {"
    "  if (t == 0.0) {"
    "    return u_curvePoints[0].xyz;"
    "  } else if (t == 1.0) {"
    "    return u_curvePoints[u_nbCurvePoints - 1].xyz;"
    "  } else {"
    "    float s = (1.0 - t);"
    "    float r = float(u_nbCurvePoints);"
    "    float curCoeff = 1.0;"
    "    float t2 = 1.0;"
    "    vec3 bezierPoint = vec3(0.0);"
    "    for (int i = 0 ; i < MAX_CURVE_POINTS ; ++i) { "
    "      bezierPoint += u_curvePoints[i].xyz * curCoeff * t2 * pow(s, float(u_nbCurvePoints - 1 - i));"
    "      float c = float(i+1);"
    "      curCoeff *= (r-c)/c;"
    "      t2 *= t;"
    "      if (i == u_nbCurvePoints) break;"
    "    }"
    "    return bezierPoint;"
    "  }"
    "}"

    "\n#endif\n"

    "\n#ifdef BSPLINE_VERTEX_SHADER\n"

    "const int bsplineDegree = 3;"
    "float coeffs[bsplineDegree + 1];"

    "vec3 computeCurvePoint(float t) {"
    "  if (t == 0.0) {"
    "    return u_curvePoints[0].xyz;"
    "  } else if (t >= 1.0) {"
    "    return u_curvePoints[u_nbCurvePoints - 1].xyz;"
    "  } else {"
    "    int k = bsplineDegree;"
    "    float cpt = 0.0;"
    "    for (int i = 0 ; i < MAX_CURVE_POINTS + bsplineDegree + 1 ; ++i) {"
    "      if (!(t > (cpt * u_stepKnots) && t >= ((cpt+1.0) * u_stepKnots))) break;"
    "      ++k;"
    "      ++cpt;"
    "    }"
    "    float knotVal = cpt * u_stepKnots;"
    "    for (int i = 0 ; i < (bsplineDegree + 1) ; ++i) {"
    "      coeffs[i] = 0.0;"
    "    }"
    "    coeffs[bsplineDegree] = 1.0;"
    "    for (int i = 1 ; i <= bsplineDegree ; ++i) {"
    "      coeffs[bsplineDegree-i] = (clamp(knotVal + u_stepKnots, 0.0, 1.0) - t) / (clamp(knotVal + u_stepKnots, 0.0, 1.0) - clamp(knotVal + float(-i+1) * u_stepKnots, 0.0, 1.0)) * coeffs[bsplineDegree-i+1];"
    "      int tabIdx = bsplineDegree-i+1;"
    "      for (int j = -bsplineDegree+1 ; j <= -1 ; ++j) {"
    "        if (j < (-i+1)) continue;"
    "        if (tabIdx == 0)"
    "          coeffs[0] = ((t - clamp(knotVal + float(j) * u_stepKnots, 0.0, 1.0)) / (clamp(knotVal + float(j+i) * u_stepKnots, 0.0, 1.0) - clamp(knotVal + float(j) * u_stepKnots, 0.0, 1.0))) * coeffs[0] + ((clamp(knotVal + float(j+i+1) * u_stepKnots, 0.0, 1.0) - t) / (clamp(knotVal + float(j+i+1) * u_stepKnots, 0.0, 1.0) - clamp(knotVal + float(j+1) * u_stepKnots, 0.0, 1.0))) * coeffs[1];"
    "        if (tabIdx == 1)"
    "          coeffs[1] = ((t - clamp(knotVal + float(j) * u_stepKnots, 0.0, 1.0)) / (clamp(knotVal + float(j+i) * u_stepKnots, 0.0, 1.0) - clamp(knotVal + float(j) * u_stepKnots, 0.0, 1.0))) * coeffs[1] + ((clamp(knotVal + float(j+i+1) * u_stepKnots, 0.0, 1.0) - t) / (clamp(knotVal + float(j+i+1) * u_stepKnots, 0.0, 1.0) - clamp(knotVal + float(j+1) * u_stepKnots, 0.0, 1.0))) * coeffs[2];"
    "        if (tabIdx == 2)"
    "          coeffs[2] = ((t - clamp(knotVal + float(j) * u_stepKnots, 0.0, 1.0)) / (clamp(knotVal + float(j+i) * u_stepKnots, 0.0, 1.0) - clamp(knotVal + float(j) * u_stepKnots, 0.0, 1.0))) * coeffs[2] + ((clamp(knotVal + float(j+i+1) * u_stepKnots, 0.0, 1.0) - t) / (clamp(knotVal + float(j+i+1) * u_stepKnots, 0.0, 1.0) - clamp(knotVal + float(j+1) * u_stepKnots, 0.0, 1.0))) * coeffs[3];"
    "        ++tabIdx;"
    "      }"
    "      coeffs[3] = ((t - knotVal) / (clamp(knotVal + float(i) * u_stepKnots, 0.0, 1.0) - knotVal)) * coeffs[3];"
    "    }"
    "    int startIdx = k - bsplineDegree;"
    "    vec3 curvePoint = vec3(0.0);"
    "    for (int i = 0 ; i <= bsplineDegree ; ++i) {"
    "      curvePoint += coeffs[i] * u_curvePoints[startIdx + i].xyz;"
    "    }"
    "    return curvePoint;"
    "  }"
    "}"

    "\n#endif\n"

    "\n#ifdef CATMULL_VERTEX_SHADER\n"

    "vec3 bezierControlPoints[4];"

    "float parameter[2];"

    "const float alpha = 0.5;"

    "void computeBezierSegmentControlPoints(vec3 pBefore, vec3 pStart, vec3 pEnd, vec3 pAfter) {"
    "  bezierControlPoints[0] = pStart;"
    "  float d1 = distance(pBefore, pStart);"
    "  float d2 = distance(pStart, pEnd);"
    "  float d3 = distance(pEnd, pAfter);"
    "  float d1alpha = pow(d1, alpha);"
    "  float d12alpha = pow(d1, 2.0*alpha);"
    "  float d2alpha = pow(d2, alpha);"
    "  float d22alpha = pow(d2, 2.0*alpha);"
    "  float d3alpha = pow(d3, alpha);"
    "  float d32alpha = pow(d3, 2.0*alpha);"
    "  bezierControlPoints[1] = (d12alpha*pEnd-d22alpha*pBefore+(2.0*d12alpha+3.0*d1alpha*d2alpha+d22alpha)*pStart)/(3.0*d1alpha*(d1alpha+d2alpha));"
    "  bezierControlPoints[2] = (d32alpha*pStart-d22alpha*pAfter+(2.0*d32alpha+3.0*d3alpha*d2alpha+d22alpha)*pEnd)/(3.0*d3alpha*(d3alpha+d2alpha));"
    "  bezierControlPoints[3] = pEnd;"
    "}"

    "int computeSegmentIndex(float t) {"
    "  float dist = pow(distance(u_curvePoints[0].xyz, u_curvePoints[1].xyz), alpha);"
    "  parameter[0] = 0.0;"
    "  parameter[1] = dist / u_curvePoints[u_nbCurvePoints-1].w;"
    "  if (t == 0.0) {"
    "    return 0;"
    "  } else if (t == 1.0)   {"
    "    return u_nbCurvePoints - 1;"
    "  } else {"
    "    int ret = 0;"
    "    for (int i = 0 ; i < MAX_CURVE_POINTS ; ++i) {"
    "      ret = i;"
    "      if (t < (dist / u_curvePoints[u_nbCurvePoints-1].w)) break;"
    "      parameter[0] = dist / u_curvePoints[u_nbCurvePoints-1].w;"
    "      dist += pow(distance(u_curvePoints[i].xyz, u_curvePoints[i+1].xyz), alpha);"
    "    }"
    "    parameter[1] = dist / u_curvePoints[u_nbCurvePoints-1].w;"
    "    return ret;"
    "  }"
    "}"

    "vec3 computeCurvePoint(float t) {"
    "  int i = computeSegmentIndex(t);"
    "  float localT = 0.0;"
    "  if (t == 1.0) {"
    "    localT = 1.0;"
    "  } else if (t != 0.0) {"
    "    localT = (t - parameter[0]) / (parameter[1] - parameter[0]);"
    "  }"
    "  if (i == 0) {"
    "    computeBezierSegmentControlPoints(u_curvePoints[i].xyz - (u_curvePoints[i+1].xyz - u_curvePoints[i].xyz), u_curvePoints[i].xyz, u_curvePoints[i+1].xyz, u_curvePoints[i+2].xyz);"
    "  } else if (i == u_nbCurvePoints - 2) {"
    "    computeBezierSegmentControlPoints(u_curvePoints[i-1].xyz, u_curvePoints[i].xyz, u_curvePoints[i+1].xyz, u_curvePoints[i+1].xyz + (u_curvePoints[i+1].xyz - u_curvePoints[i].xyz));"
    "  } else if (i == u_nbCurvePoints - 1) {"
    "    computeBezierSegmentControlPoints(u_curvePoints[i-2].xyz, u_curvePoints[i-1].xyz, u_curvePoints[i].xyz, u_curvePoints[i].xyz + (u_curvePoints[i].xyz - u_curvePoints[i-1].xyz));"
    "  } else {"
    "    computeBezierSegmentControlPoints(u_curvePoints[i-1].xyz, u_curvePoints[i].xyz, u_curvePoints[i+1].xyz, u_curvePoints[i+2].xyz);"
    "  }"
    "  float t2 = localT * localT;"
    "  float t3 = t2 * localT;"
    "  float s = 1.0 - localT;"
    "  float s2 = s * s;"
    "  float s3 = s2 * s;"
    "  return (bezierControlPoints[0] * s3 + bezierControlPoints[1] * 3.0 * localT * s2 + bezierControlPoints[2] * 3.0 * t2 * s + bezierControlPoints[3] * t3);"
    "}"

    "\n#endif\n"

    "vec3 computeBinormal(vec3 nextCurvePoint, vec3 currentCurvePoint) {"
    "  vec3 tangent = normalize(nextCurvePoint - currentCurvePoint);"
    "  vec3 normal = vec3(-tangent.y, tangent.x, tangent.z);"
    "  if (tangent.x == 0.0 && tangent.y == 0.0) {"
    "    normal = vec3(-tangent.z, tangent.x, tangent.y);"
    "  }"
    "  vec3 ret = cross(tangent, normal);"
    "  return ret;"
    "}"

    "vec3 computeCurvePoint(vec3 previousCurvePoint, vec3 currentCurvePoint, vec3 nextCurvePoint, float ori, float size, vec3 lookDir) {"
    "  vec3 pos = vec3(0.0);"
    "  vec3 u = previousCurvePoint - currentCurvePoint;"
    "  vec3 v = nextCurvePoint - currentCurvePoint;"
    "  vec3 un = normalize(u);"
    "  vec3 vn = normalize(v);"
    "  vec3 xu = normalize(cross(un, lookDir));"
    "  vec3 xv = normalize(cross(vn, -lookDir));"
    "  vec3 xu_xv = normalize(xu+xv);"
    //"  float angle = M_PI - acos(dot(u,v)/length(u)*length(v));"
    //"  if (angle != angle) angle = 0.0;"
    //"  size = size/cos(angle/2.0);"
    "  pos = currentCurvePoint + ori * xu_xv * size;"
    "  return pos;"
    "}"

    "void main() {"
    "  int pointIdx = 0;"
    "  vec3 currentCurvePoint = vec3(0.0);"
    "  vec3 previousCurvePoint = vec3(0.0);"
    "  vec3 nextCurvePoint = vec3(0.0);"
    "  float size = 0.0;"
    "  float s = 0.0;"
    "\n#ifdef POLYLINE_VERTEX_SHADER\n"
    "    pointIdx = int(a_position.x);"
    "    currentCurvePoint = u_curvePoints[pointIdx].xyz;"
    "    if (pointIdx == 0) {"
    "      nextCurvePoint = u_curvePoints[pointIdx+1].xyz;"
    "      previousCurvePoint = currentCurvePoint - (nextCurvePoint - currentCurvePoint);"
    "    } else if (pointIdx == u_nbCurvePoints-1) {"
    "      previousCurvePoint = u_curvePoints[pointIdx-1].xyz;"
    "      nextCurvePoint = currentCurvePoint + (currentCurvePoint - previousCurvePoint);"
    "    } else {"
    "      previousCurvePoint = u_curvePoints[pointIdx-1].xyz;"
    "      nextCurvePoint = u_curvePoints[pointIdx+1].xyz;"
    "    }"
    "    float t = u_curvePoints[pointIdx].w / u_curvePoints[u_nbCurvePoints - 1].w;"
    "    v_color = mix(u_startColor, u_endColor, t);"
    "    size = mix(u_startSize, u_endSize, t);"
    "    s = u_curvePoints[pointIdx].w/(u_startSize*2.0);"
    "\n#else\n"
    "    float t = a_position.x;"
    "    currentCurvePoint = computeCurvePoint(t);"
    "    if (t == 0.0) {"
    "      nextCurvePoint = computeCurvePoint(t + u_curveInterpolationStep);"
    "      previousCurvePoint = currentCurvePoint - (nextCurvePoint - currentCurvePoint);"
    "    } else if (t == 1.0) {"
    "      previousCurvePoint = computeCurvePoint(t - u_curveInterpolationStep);"
    "      nextCurvePoint = currentCurvePoint + (currentCurvePoint - previousCurvePoint);"
    "    } else {"
    "      previousCurvePoint = computeCurvePoint(t - u_curveInterpolationStep);"
    "      nextCurvePoint = computeCurvePoint(t + u_curveInterpolationStep);"
    "    }"
    "    v_color = mix(u_startColor, u_endColor, t);"
    "    size = mix(u_startSize, u_endSize, t);"
    "    if (u_billboard || u_textureActivated) {"
    "      float t1 = float(u_nbCurvePoints/2) / float(u_nbCurvePoints - 1);"
    "      float t2 = float(u_nbCurvePoints/2 + 1) / float(u_nbCurvePoints - 1);"
    "      float texCoordFactor = distance(computeCurvePoint(t1), computeCurvePoint(t2))/(u_startSize*2.0);"
    "      s = t * float(u_nbCurvePoints - 1) * texCoordFactor;"
    "    }"
    "\n#endif\n"
    "  if (u_lineMode) {"
    "    gl_Position = u_projectionMatrix * u_modelviewMatrix * vec4(currentCurvePoint, 1.0);"
    "  } else {"
    "    float ori = a_position.y;"
    "    vec3 binormal = u_lookDir;"
    "    if (!u_billboard) {"
    "      binormal = computeBinormal(currentCurvePoint, nextCurvePoint);"
    "    }"
    "    vec3 pos = computeCurvePoint(previousCurvePoint, currentCurvePoint, nextCurvePoint, ori, size, binormal);"
    "    gl_Position = u_projectionMatrix * u_modelviewMatrix * vec4(pos, 1.0);"
    "    if (ori < 0.0) {"
    "      v_texCoord = vec2(s, 0.0);"
    "    } else {"
    "      v_texCoord = vec2(s, 1.0);"
    "    }"
    "  }"
    "}"
    ;

static string curveFragmentShaderSrc =
    #ifdef __EMSCRIPTEN__
    "precision highp float;\n"
    "precision highp int;\n"
    #else
    "#version 120\n"
    #endif
    "uniform sampler2D u_texture0;"
    "uniform sampler2D u_texture1;"
    "uniform bool u_textureActivated;"
    "uniform bool u_billboardTex;"

    "varying vec4 v_color;"
    "varying vec2 v_texCoord;"

    "void main() {"
    "  gl_FragColor = v_color;"
    "  if (u_billboardTex) {"
    "    gl_FragColor *= texture2D(u_texture0, v_texCoord);"
    "  }"
    "  if (u_textureActivated) {"
    "    gl_FragColor *= texture2D(u_texture1, v_texCoord);"
    "  }"
    "}"
    ;

const unsigned int nbCurveInterpolationPoints = 200;
const unsigned int bSplineDegree = 3;
const float alpha = 0.5f;

struct NodesLabelsSorting {

  NodesLabelsSorting(BooleanProperty *selection, NumericProperty *metric = NULL) : _selection(selection), _metric(metric) {}

  bool operator()(const tlp::node &n1, const tlp::node &n2) const {
    if (_selection->getNodeValue(n1) && !_selection->getNodeValue(n2)) {
      return true;
    } else if (!_selection->getNodeValue(n1) && _selection->getNodeValue(n2)) {
      return false;
    } else if (_metric) {
      return (_metric->getNodeDoubleValue(n1) > _metric->getNodeDoubleValue(n2));
    } else {
      return n1.id < n2.id;
    }
  }

private:

  BooleanProperty *_selection;
  NumericProperty *_metric;

};


class GreatThanNode {

public:

  GreatThanNode(NumericProperty *metric) : _metric(metric) {}

  bool operator() (const node &n1, const node &n2) const {
    return (_metric->getNodeDoubleValue(n1) > _metric->getNodeDoubleValue(n2));
  }

private:

  NumericProperty *_metric;

};

class GreatThanEdge {

public:

  GreatThanEdge(NumericProperty *metric) : _metric(metric) {}

  bool operator() (const edge &e1, const edge &e2) const {
    return (_metric->getEdgeDoubleValue(e1) > _metric->getEdgeDoubleValue(e2));
  }

private:

  NumericProperty *_metric;

};

GlShaderProgram *GlGraph::getEdgeShader(int edgeShape) {
  if (_edgesShaders.find(edgeShape) == _edgesShaders.end()) {
    string preproDefine =
    #ifdef __EMSCRIPTEN__
        "precision highp float;\n"
        "precision highp int;\n"
    #else
        "#version 120\n"
    #endif
        ;
    if (edgeShape == EdgeShape::Polyline) {
      preproDefine += "#define POLYLINE_VERTEX_SHADER\n";
    } else if (edgeShape == EdgeShape::BezierCurve) {
      preproDefine += "#define BEZIER_VERTEX_SHADER\n";
    } else if (edgeShape == EdgeShape::CatmullRomCurve) {
      preproDefine += "#define CATMULL_VERTEX_SHADER\n";
    } else {
      preproDefine += "#define BSPLINE_VERTEX_SHADER\n";
    }
    GlShaderProgram *edgeShader = new GlShaderProgram();
    edgeShader->addShaderFromSourceCode(GlShader::Vertex, preproDefine + curveVertexShaderSrc);
    edgeShader->addShaderFromSourceCode(GlShader::Fragment, curveFragmentShaderSrc);
    edgeShader->link();
    edgeShader->printInfoLog();
    _edgesShaders[edgeShape] = edgeShader;
  }
  return _edgesShaders[edgeShape];
  return NULL;
}

GlGraph::GlGraph(Graph *graph, GlLODCalculator *lodCalculator) :
  _graph(NULL),
  _viewColor(NULL),
  _viewLayout(NULL),
  _viewSize(NULL),
  _viewRotation(NULL),
  _viewBorderWidth(NULL),
  _viewBorderColor(NULL),
  _viewLabel(NULL),
  _viewShape(NULL),
  _viewTexture(NULL),
  _viewSelection(NULL),
  _canUseUIntIndices(true),
  _graphElementsPickingMode(false),
  _lodCalculator(lodCalculator),
  _updateQuadTree(true)
{

#ifdef __EMSCRIPTEN__
  std::string webglExtensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
  _canUseUIntIndices = webglExtensions.find("OES_element_index_uint") != std::string::npos;
#endif

  _flatShader = ShaderManager::getInstance()->getFlatRenderingShader();

  _pointsDataBuffer = new GlBuffer(GlBuffer::VertexBuffer, GlBuffer::DynamicDraw);

  _edgeRenderingDataBuffer = new GlBuffer(GlBuffer::VertexBuffer);
  _edgeIndicesBuffer = new GlBuffer(GlBuffer::IndexBuffer);
  _curveEdgeRenderingDataBuffer = new GlBuffer(GlBuffer::VertexBuffer);
  _curveEdgeIndicesBuffer = new GlBuffer(GlBuffer::IndexBuffer);
  _edgeLineRenderingDataBuffer = new GlBuffer(GlBuffer::VertexBuffer);
  _edgeLineRenderingIndicesBuffer = new GlBuffer(GlBuffer::IndexBuffer);

  const map<int, Glyph*> &glyphs = GlyphsManager::getGlyphs();
  map<int, Glyph*>::const_iterator it = glyphs.begin();
  for( ; it != glyphs.end() ; ++it) {
    _nodesGlyphs[it->first] = vector<node>();

  }

  _labelsRenderer = LabelsRenderer::instance();

  if (!_lodCalculator)
    _lodCalculator = new GlQuadTreeLODCalculator();

  _lodCalculator->setRenderingEntitiesFlag(RenderingNodesEdges);

  _renderingParameters.setGlGraph(this);
  _renderingParameters.addListener(this);
  _renderingParameters.addObserver(this);

  setGraph(graph);

}

void GlGraph::setRenderingParameters(const GlGraphRenderingParameters &renderingParameters) {
  _renderingParameters = renderingParameters;
  _renderingParameters.setGlGraph(this);
  _renderingParameters.addListener(this);
  _renderingParameters.addObserver(this);
}

GlGraph::~GlGraph() {

  delete _pointsDataBuffer;
  delete _edgeRenderingDataBuffer;
  delete _edgeIndicesBuffer;
  delete _curveEdgeRenderingDataBuffer;
  delete _curveEdgeIndicesBuffer;
  delete _edgeLineRenderingDataBuffer;
  delete _edgeLineRenderingIndicesBuffer;
  delete _lodCalculator;
}

void GlGraph::setGraph(tlp::Graph *graph) {

  if (_graph == graph || !graph) {
    return;
  }

  if (_graph) {
    clearObservers();
  }

  _graph = graph;

  initGraphProperties();

  _lastGraphBoundingBox = BoundingBox();

  prepareNodesLabelsData();

  initObservers();

  computeGraphBoundingBox();

  _edgesToUpdate.clear();
  _nodesToUpdate.clear();
  _edgePoints.clear();
  _edgeLineVerticesIndices.clear();

  prepareEdgesData();

  _updateQuadTree = true;

  notifyModified();

}

void GlGraph::initGraphProperties() {
  _viewColor = _graph->getProperty<ColorProperty>("viewColor");
  _viewLayout = _graph->getProperty<LayoutProperty>("viewLayout");
  _viewSize = _graph->getProperty<SizeProperty>("viewSize");
  _viewRotation = _graph->getProperty<DoubleProperty>("viewRotation");
  _viewBorderWidth = _graph->getProperty<DoubleProperty>("viewBorderWidth");
  _viewBorderColor = _graph->getProperty<ColorProperty>("viewBorderColor");
  _viewLabel = _graph->getProperty<StringProperty>("viewLabel");
  _viewShape = _graph->getProperty<IntegerProperty>("viewShape");
  _viewTexture = _graph->getProperty<StringProperty>("viewTexture");
  _viewSelection = _graph->getProperty<BooleanProperty>("viewSelection");
  _viewSrcAnchorShape = _graph->getProperty<IntegerProperty>("viewSrcAnchorShape");
  _viewTgtAnchorShape = _graph->getProperty<IntegerProperty>("viewTgtAnchorShape");
  _viewSrcAnchorSize = _graph->getProperty<SizeProperty>("viewSrcAnchorSize");
  _viewTgtAnchorSize = _graph->getProperty<SizeProperty>("viewTgtAnchorSize");
  _viewFontAwesomeIcon = _graph->getProperty<StringProperty>("viewFontAwesomeIcon");
  _viewGlow = _graph->getProperty<BooleanProperty>("viewGlow");
  _viewLabelColor = _graph->getProperty<ColorProperty>("viewLabelColor");
}

static const string viewPropPrefix = "view";

void GlGraph::initObservers() {
  _graph->addListener(this);
  _graph->addObserver(this);
  string propName;
  forEach(propName, _graph->getProperties()) {
    if (propName.length() > viewPropPrefix.length() && propName.substr(0, viewPropPrefix.length()) == viewPropPrefix) {
      PropertyInterface *prop = _graph->getProperty(propName);
      prop->addListener(this);
      prop->addObserver(this);
    }
  }
}

void GlGraph::clearObservers() {
  _graph->removeListener(this);
  _graph->removeObserver(this);
  string propName;
  forEach(propName, _graph->getProperties()) {
    if (propName.length() > viewPropPrefix.length() && propName.substr(0, viewPropPrefix.length()) == viewPropPrefix) {
      PropertyInterface *prop = _graph->getProperty(propName);
      prop->removeListener(this);
      prop->removeObserver(this);
    }
  }
}

void GlGraph::computeGraphBoundingBox() {
  _boundingBox = tlp::computeBoundingBox(_graph, _viewLayout, _viewSize, _viewRotation);
}

void GlGraph::prepareEdgesData() {

  if (_graph->numberOfEdges() == 0) return;

  startEdgesData();

  edge e;
  forEach(e, _graph->getEdges()) {
    addEdgeData(e);
  }

  endEdgesData();

}

void GlGraph::startEdgesData() {
  _srcEdgeExtremitiesData.clear();
  _tgtEdgeExtremitiesData.clear();
  _maxEdgePoints = 0;
}

void GlGraph::addEdgeData(const tlp::edge e) {
  prepareEdgeData(e);
}

void GlGraph::endEdgesData() {
  vector<Coord> edgeVerticesData;
  vector<unsigned short> edgeIndices;
  for (size_t i = 0 ; i < _maxEdgePoints ; ++i) {
    edgeVerticesData.push_back(Coord(i, -1));
    edgeVerticesData.push_back(Coord(i, 1));
  }
  for (size_t i = 0 ; i < _maxEdgePoints ; ++i) {
    edgeIndices.push_back(2*i);
    edgeIndices.push_back(2*i+1);
  }
  for (size_t i = 0 ; i < _maxEdgePoints ; ++i) {
    edgeIndices.push_back(2*i);
  }
  for (size_t i = 0 ; i < _maxEdgePoints ; ++i) {
    edgeIndices.push_back(2*i+1);
  }

  _edgeRenderingDataBuffer->allocate(edgeVerticesData);
  _edgeIndicesBuffer->allocate(edgeIndices);

  edgeVerticesData.clear();
  edgeIndices.clear();

  for (size_t i = 0 ; i < nbCurveInterpolationPoints ; ++i) {
    edgeVerticesData.push_back(Coord(i/static_cast<float>(nbCurveInterpolationPoints-1), -1));
    edgeVerticesData.push_back(Coord(i/static_cast<float>(nbCurveInterpolationPoints-1), 1));
  }
  for (size_t i = 0 ; i < nbCurveInterpolationPoints ; ++i) {
    edgeIndices.push_back(2*i);
    edgeIndices.push_back(2*i+1);
  }
  for (size_t i = 0 ; i < nbCurveInterpolationPoints ; ++i) {
    edgeIndices.push_back(2*i);
  }
  for (size_t i = 0 ; i < nbCurveInterpolationPoints ; ++i) {
    edgeIndices.push_back(2*i+1);
  }

  _curveEdgeRenderingDataBuffer->allocate(edgeVerticesData);
  _curveEdgeIndicesBuffer->allocate(edgeIndices);

  vector<float> edgesLinesRenderingData;
  edge e;
  forEach(e, _graph->getEdges()) {

    Color edgeColor = _viewColor->getEdgeValue(e);
    const Color &srcColor = _viewColor->getNodeValue(_graph->source(e));
    const Color &tgtColor = _viewColor->getNodeValue(_graph->target(e));
    const Color &selectionColor = uintToColor(_graph->numberOfNodes() + e.id + 1);

    if (_viewBorderWidth->getEdgeValue(e) > 0) {
      edgeColor = _viewBorderColor->getEdgeValue(e);
    }

    vector<Coord> edgePoints;
    edgePoints.push_back(_edgeAnchors[e].first);
    edgePoints.insert(edgePoints.end(), _edgePoints[e].begin(), _edgePoints[e].end());
    edgePoints.push_back(_edgeAnchors[e].second);

    vector<Color> edgeColors;
    getColors(edgePoints, srcColor, tgtColor, edgeColors);

    std::vector<unsigned int> lineIndices;

    for (size_t i = 0 ; i < edgePoints.size() ; ++i) {
      unsigned int currentNbVertices = edgesLinesRenderingData.size() / 15;
      if (i != edgePoints.size() - 1) {
        lineIndices.push_back(currentNbVertices);
        lineIndices.push_back(currentNbVertices+1);
      }
      addTlpVecToVecFloat(edgePoints[i], edgesLinesRenderingData);
      addColorToVecFloat(edgeColor, edgesLinesRenderingData);
      addColorToVecFloat(edgeColors[i], edgesLinesRenderingData);
      addColorToVecFloat(selectionColor, edgesLinesRenderingData);
    }
    _edgeLineVerticesIndices[e] = lineIndices;
  }
  _edgeLineRenderingDataBuffer->allocate(edgesLinesRenderingData);


  _updateQuadTree = true;

}

void GlGraph::prepareNodesLabelsData() {
  _labelsRenderer->initFont();
  if (_labelsRenderer->fontInit()) {
    _labelsRenderer->clearGraphNodesLabelsRenderingData(_graph);
    if (_graph->numberOfNodes() > 0) {
      node n;
      forEach(n, _graph->getNodes()) {
        prepareNodeLabelData(n);
      }
    }
  }
}

void GlGraph::prepareNodeLabelData(const tlp::node n) {
  _labelsRenderer->addOrUpdateNodeLabel(_graph, n);
}

void GlGraph::draw(const Camera &camera, const Light &light, bool pickingMode) {

  if (!_graph || _graph->numberOfNodes() == 0) return;

  if (pickingMode) {
    GlyphsRenderer::instance()->renderGlyph(camera, light, NodeShape::Cube, _boundingBox.center(),
                                            Size(_boundingBox.width(), _boundingBox.height(), _boundingBox.depth()),
                                            _pickingColor, "", 0, tlp::Color(), tlp::Vec4f(), true);
    return;
  }

  if (_updateQuadTree) {
    _lodCalculator->setSceneBoundingBox(_boundingBox);
    _lodCalculator->setGraph(_graph, &_renderingParameters);
    _lastGraphBoundingBox = _boundingBox;
    _updateQuadTree = false;
  }

  if (!_graphElementsPickingMode) {
    _lodCalculator->compute(const_cast<Camera*>(&camera));
  } else {
    _lodCalculator->compute(const_cast<Camera*>(&camera), _selectionViewport);
  }

  _nodesGlyphs.clear();

  vector<node> nodesLabelsToRender;
  vector<node> selectedNodes;
  vector<node> pointsNodes;
  vector<node> glyphsNodes;
  vector<node> metaNodes;
  selectedNodes.reserve(_graph->numberOfNodes());
  pointsNodes.reserve(_graph->numberOfNodes());
  const vector<NodeEntityLODUnit> &nodesLodResult = _lodCalculator->getNodesResult();

  for (size_t i = 0 ; i < nodesLodResult.size() ; ++i) {

    if (nodesLodResult[i].lod < 0 || _nodesToDiscard.find(nodesLodResult[i].n) != _nodesToDiscard.end()) continue;
    if (nodesLodResult[i].lod < 10 && !_renderingParameters.bypassLodSystem()) {
      pointsNodes.push_back(nodesLodResult[i].n);
      if (!_renderingParameters.labelsScaled()) {
        nodesLabelsToRender.push_back(nodesLodResult[i].n);
      }
      continue;
    }
    nodesLabelsToRender.push_back(nodesLodResult[i].n);
    glyphsNodes.push_back(nodesLodResult[i].n);
    int glyphId = _viewShape->getNodeValue(nodesLodResult[i].n);
    if (glyphId == tlp::NodeShape::FontAwesomeIcon) {
      std::string icon = _viewFontAwesomeIcon->getNodeValue(nodesLodResult[i].n);
      glyphId = tlp::TulipFontAwesome::getFontAwesomeIconCodePoint(icon);
    }
    _nodesGlyphs[glyphId].push_back(nodesLodResult[i].n);
    if (_viewSelection->getNodeValue(nodesLodResult[i].n) && nodesLodResult[i].lod >= 10) {
      selectedNodes.push_back(nodesLodResult[i].n);
    }
    if (_graph->isMetaNode(nodesLodResult[i].n)) {
      metaNodes.push_back(nodesLodResult[i].n);
    }
  }

  vector<edge> pointsEdges;
  vector<edge> quadsEdges, selectedQuadsEdges;
  vector<edge> lineEdges, selectedLinesEdges;

  quadsEdges.reserve(_graph->numberOfEdges());
  selectedQuadsEdges.reserve(_graph->numberOfEdges());
  lineEdges.reserve(_graph->numberOfEdges());
  selectedLinesEdges.reserve(_graph->numberOfEdges());
  const vector<EdgeEntityLODUnit> &edgesLodResult = _lodCalculator->getEdgesResult();

  for (size_t i = 0 ; i < edgesLodResult.size() ; ++i) {
    if (edgesLodResult[i].lod < 0 || _edgesToDiscard.find(edgesLodResult[i].e) != _edgesToDiscard.end()) continue;
    if (edgesLodResult[i].lod < 5 && !_renderingParameters.bypassLodSystem()) {
      pointsEdges.push_back(edgesLodResult[i].e);
    } else if (abs(edgesLodResult[i].lodSize) < 5 && !_renderingParameters.bypassLodSystem()) {
      if (_viewSelection->getEdgeValue(edgesLodResult[i].e)) {
        selectedLinesEdges.push_back(edgesLodResult[i].e);
      } else {
        lineEdges.push_back(edgesLodResult[i].e);
      }
    }
    else {
      if (_viewSelection->getEdgeValue(edgesLodResult[i].e)) {
        selectedQuadsEdges.push_back(edgesLodResult[i].e);
      } else {
        quadsEdges.push_back(edgesLodResult[i].e);
      }
    }
  }

  if (_graphElementsPickingMode) {
    glDisable(GL_BLEND);
  } else if (_renderingParameters.displayMetaNodes()) {
    renderMetaNodes(metaNodes, camera, light);
  }

  NumericProperty *orderingProperty = _renderingParameters.elementsOrderingProperty();
  GreatThanEdge gte(orderingProperty);
  GreatThanNode gtn(orderingProperty);

  if (_renderingParameters.elementsOrdered() && orderingProperty) {

    std::sort(pointsEdges.begin(), pointsEdges.end(), gte);
    std::sort(glyphsNodes.begin(), glyphsNodes.end(), gtn);
    std::sort(pointsNodes.begin(), pointsNodes.end(), gtn);

    if (!_renderingParameters.elementsOrderedDescending()) {
      std::reverse(pointsEdges.begin(), pointsEdges.end());
      std::reverse(glyphsNodes.begin(), glyphsNodes.end());
      std::reverse(pointsNodes.begin(), pointsNodes.end());
    }

  }

  if (_renderingParameters.displayEdges()) {

    if (_renderingParameters.elementsOrdered() && _renderingParameters.elementsOrderingProperty()) {
      set<edge> lineEdgesSet(lineEdges.begin(), lineEdges.end());
      vector<edge> edgesToRender = lineEdges;
      edgesToRender.insert(edgesToRender.end(), quadsEdges.begin(), quadsEdges.end());
      std::sort(edgesToRender.begin(), edgesToRender.end(), gte);
      if (!_renderingParameters.elementsOrderedDescending()) {
        std::reverse(edgesToRender.begin(), edgesToRender.end());
      }
      vector<edge> currentEdgesRenderingBash;
      currentEdgesRenderingBash.push_back(edgesToRender.front());
      bool lineEdges = lineEdgesSet.find(edgesToRender.front()) != lineEdgesSet.end();
      for (size_t i = 1 ; i < edgesToRender.size() ; ++i) {
        bool lineEdge = lineEdgesSet.find(edgesToRender[i]) != lineEdgesSet.end();
        if (lineEdge == lineEdges) {
          currentEdgesRenderingBash.push_back(edgesToRender[i]);
        } else {
          if (lineEdges) {
            renderEdges(camera, light, currentEdgesRenderingBash, true);
          } else {
            if (!_renderingParameters.edges3D()) {
              renderEdges(camera, light, currentEdgesRenderingBash, false, false);
            } else {
              renderEdges(camera, light, currentEdgesRenderingBash, false, true);
            }
          }
          currentEdgesRenderingBash.clear();
          currentEdgesRenderingBash.push_back(edgesToRender[i]);
          lineEdges = lineEdge;
        }
      }
      if (!currentEdgesRenderingBash.empty()) {
        if (lineEdges) {
          renderEdges(camera, light, currentEdgesRenderingBash, true);
        } else {
          if (!_renderingParameters.edges3D()) {
            renderEdges(camera, light, currentEdgesRenderingBash, false, false);
          } else {
            renderEdges(camera, light, currentEdgesRenderingBash, false, true);
          }
        }
      }
    } else {
      renderEdges(camera, light, lineEdges, true);

      if (!_renderingParameters.edges3D()) {
        renderEdges(camera, light, quadsEdges, false, false);
      } else {
        renderEdges(camera, light, quadsEdges, false, true);
      }
    }

    renderEdges(camera, light, selectedLinesEdges, true);

    if (!_renderingParameters.edges3D()) {
      renderEdges(camera, light, selectedQuadsEdges, false, false);
    } else {
      renderEdges(camera, light, selectedQuadsEdges, false, true);
    }

  }

  renderNodesGlow(glyphsNodes, camera, light);

  if (_renderingParameters.displayNodes()) {
    renderNodes(camera, light);
  }

  renderPointsNodesAndEdges(camera, pointsNodes, pointsEdges);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);
  if (!_graphElementsPickingMode && _renderingParameters.displayNodesLabels()) {
    _labelsRenderer->setLabelsScaled(_renderingParameters.labelsScaled());
    _labelsRenderer->setMinMaxSizes(_renderingParameters.minSizeOfLabels(), _renderingParameters.maxSizeOfLabels());
    std::sort(nodesLabelsToRender.begin(), nodesLabelsToRender.end(), NodesLabelsSorting(_viewSelection, orderingProperty));
    if (_renderingParameters.elementsOrdered() && orderingProperty && !_renderingParameters.elementsOrderedDescending()) {
      vector<node>::iterator itN = nodesLabelsToRender.begin();
      while (itN != nodesLabelsToRender.end()) {
        if (_viewSelection->getNodeValue(*itN)) {
          break;
        } else {
          ++itN;
        }
      }
      std::reverse(nodesLabelsToRender.begin(), itN);
    }
    _labelsRenderer->setGraphNodesLabelsToRender(_graph, nodesLabelsToRender);
    _labelsRenderer->renderGraphNodesLabels(_graph, camera, _renderingParameters.selectionColor());
  }
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);

  if (_graphElementsPickingMode) {
    glEnable(GL_BLEND);
  }

}

void GlGraph::renderMetaNodes(const std::vector<tlp::node> &metaNodes, const Camera &camera, const Light &) {
  GlScene subScene;
  subScene.setBackupBackBuffer(false);
  GlGraph *glMetaGraph = new GlGraph(NULL, new GlCPULODCalculator());
  subScene.getMainLayer()->addGlEntity(glMetaGraph, "graph");

  Coord eyeDirection = camera.getEyes()-camera.getCenter();
  eyeDirection /= eyeDirection.norm();

  for (size_t i = 0 ; i < metaNodes.size() ; ++i) {
    glMetaGraph->setGraph(_graph->getNodeMetaInfo(metaNodes[i]));
    const tlp::Coord &metaNodePos = _viewLayout->getNodeValue(metaNodes[i]);
    const tlp::Size &metaNodeSize = _viewSize->getNodeValue(metaNodes[i]);
    tlp::BoundingBox metaNodeBB(metaNodePos - metaNodeSize/2.f, metaNodePos + metaNodeSize/2.f);
    Glyph *metaNodeGlyph = GlyphsManager::getGlyph(_viewShape->getNodeValue(metaNodes[i]));
    tlp::BoundingBox includeBB;
    metaNodeGlyph->getIncludeBoundingBox(includeBB);
    tlp::BoundingBox metaNodeBBTmp;
    metaNodeBBTmp[0]=metaNodeBB.center()-Coord((metaNodeBB.width()/2.f)*(includeBB[0][0]*-2.f),(metaNodeBB.height()/2.f)*(includeBB[0][1]*-2.f),(metaNodeBB.depth()/2.f)*(includeBB[0][2]*-2.f));
    metaNodeBBTmp[1]=metaNodeBB.center()+Coord((metaNodeBB.width()/2.f)*(includeBB[1][0]*2.f),(metaNodeBB.height()/2.f)*(includeBB[1][1]*2.f),(metaNodeBB.depth()/2.f)*(includeBB[1][2]*2.f));
    metaNodeBB=metaNodeBBTmp;

    Camera newCamera2=camera;
    newCamera2.setEyes(newCamera2.getCenter()+Coord(0,0,1)*(newCamera2.getEyes()-newCamera2.getCenter()).norm());
    newCamera2.setUp(Coord(0,1,0));

    Coord first=newCamera2.worldTo2DScreen(metaNodeBB[0]);
    Coord second=newCamera2.worldTo2DScreen(metaNodeBB[1]);
    Coord center=const_cast<Camera&>(camera).worldTo2DScreen(metaNodeBB.center());
    Coord size=second-first;

    Vec4i viewport;
    viewport[0]=center[0]-size[0]/2;
    viewport[1]=center[1]-size[1]/2;
    viewport[2]=size[0];
    viewport[3]=size[1];

    viewport[0]=camera.getViewport()[0]+viewport[0]-viewport[2]/2;
    viewport[1]=camera.getViewport()[1]+viewport[1]-viewport[3]/2;
    viewport[2]*=2;
    viewport[3]*=2;

    if(viewport[2]==0 || viewport[3]==0)
      continue;

    subScene.setViewport(viewport);
    subScene.setClearBufferAtDraw(false);
    subScene.centerScene();

    Camera *subSceneCamera = subScene.getMainLayer()->getCamera();

    float baseNorm = (subSceneCamera->getEyes()-subSceneCamera->getCenter()).norm();
    subSceneCamera->setUp(camera.getUp());
    subSceneCamera->setEyes(subSceneCamera->getCenter()+(eyeDirection*baseNorm));
    subSceneCamera->setZoomFactor(subSceneCamera->getZoomFactor()*0.5);

    // small hack to avoid z-fighting between the rendering of the metanode content
    // and the rendering of the metanode that occurs afterwards
    glDepthRange(0.1, 1);
    subScene.draw();
    glDepthRange(0, 1);
  }
  if (getLayer()) {
    getLayer()->getScene()->initGlParameters();
  } else {
    tlp::Vec4i viewport = camera.getViewport();
    subScene.setViewport(viewport);
    subScene.initGlParameters();
  }

}

void GlGraph::renderNodes(const Camera &camera, const Light &light) {

  glStencilFunc(GL_LEQUAL, _renderingParameters.nodesStencil(), 0xFF);

  vector<Coord> centers;
  vector<Size> sizes;
  vector<Vec4f> rotationAngles;
  vector<Color> colors;
  vector<string> textures;
  vector<float> borderWidths;
  vector<Color> borderColors;

  vector<Coord> centersSelected;
  vector<Size> sizesSelected;
  vector<Vec4f> rotationAnglesSelected;
  vector<Color> colorsSelected;
  vector<string> texturesSelected;
  vector<float> borderWidthsSelected;
  vector<Color> borderColorsSelected;

  map<int, vector<node> >::const_iterator it = _nodesGlyphs.begin();
  for( ; it != _nodesGlyphs.end() ; ++it) {
    centers.reserve(it->second.size());
    sizes.reserve(it->second.size());
    rotationAngles.reserve(it->second.size());
    colors.reserve(it->second.size());
    textures.reserve(it->second.size());
    borderWidths.reserve(it->second.size());
    borderColors.reserve(it->second.size());
    for (size_t i = 0 ; i < it->second.size(); ++i) {
      centers.push_back(_viewLayout->getNodeValue(it->second[i]));
      sizes.push_back(_viewSize->getNodeValue(it->second[i]));
      rotationAngles.push_back(Vec4f(0.0f, 0.0f, 1.0f, _viewRotation->getNodeValue(it->second[i])));
      if (!_graphElementsPickingMode) {
        colors.push_back(_viewColor->getNodeValue(it->second[i]));
        borderColors.push_back(_viewBorderColor->getNodeValue(it->second[i]));
        textures.push_back(_viewTexture->getNodeValue(it->second[i]));
        TextureManager::instance()->addTextureInAtlasFromFile(textures.back());
      } else {
        Color pickColor = uintToColor(it->second[i].id+1);
        colors.push_back(pickColor);
        borderColors.push_back(pickColor);
        textures.push_back("");
      }
      borderWidths.push_back(_viewBorderWidth->getNodeValue(it->second[i]));

      if (!_graphElementsPickingMode && _viewSelection->getNodeValue(it->second[i])) {
        centersSelected.push_back(_viewLayout->getNodeValue(it->second[i]));
        sizesSelected.push_back(_viewSize->getNodeValue(it->second[i]));
        rotationAnglesSelected.push_back(Vec4f(0.0f, 0.0f, 1.0f, 0.0f));
        colorsSelected.push_back(Color());
        texturesSelected.push_back("");
        borderWidthsSelected.push_back(2.0f);
        borderColorsSelected.push_back(_renderingParameters.selectionColor());
      }
    }

    if (_renderingParameters.billboardedNodes()) {
      GlyphsRenderer::instance()->setBillboardMode(true);
    }
    GlyphsRenderer::instance()->renderGlyphs(camera, light, it->first, centers, sizes, colors, textures, borderWidths, borderColors, rotationAngles, _graphElementsPickingMode);
    GlyphsRenderer::instance()->setBillboardMode(false);

    centers.clear();
    sizes.clear();
    rotationAngles.clear();
    colors.clear();
    textures.clear();
    borderWidths.clear();
    borderColors.clear();
  }

  if (!_graphElementsPickingMode) {
    glDisable(GL_STENCIL_TEST);
    GlyphsRenderer::instance()->renderGlyphs(camera, light, tlp::NodeShape::CubeOutlinedTransparent, centersSelected, sizesSelected, colorsSelected, texturesSelected, borderWidthsSelected, borderColorsSelected, rotationAnglesSelected);
    glEnable(GL_STENCIL_TEST);
  }

}

void GlGraph::renderNodesGlow(const vector<node> &nodes, const Camera &camera, const Light &light) {

  if (_graphElementsPickingMode) {
    return;
  }

  glStencilMask(0x00);
  glDepthMask(GL_FALSE);

  vector<Coord> centers;
  vector<Size> sizes;
  vector<Color> colors;
  vector<string> textures;

  centers.reserve(_graph->numberOfNodes());
  sizes.reserve(_graph->numberOfNodes());
  colors.reserve(_graph->numberOfNodes());
  textures.reserve(_graph->numberOfNodes());


  for (size_t i = 0 ; i < nodes.size() ; ++i) {
    node n = nodes[i];
    if (!_viewGlow->getNodeValue(n)) continue;
    centers.push_back(_viewLayout->getNodeValue(n));
    sizes.push_back(_viewSize->getNodeValue(n) * 2.f);
    Color nodeColor = _viewColor->getNodeValue(n);
    nodeColor.setA(128);
    colors.push_back(nodeColor);
    textures.push_back("resources/radialGradientTexture.png");
    TextureManager::instance()->addTextureInAtlasFromFile("resources/radialGradientTexture.png");
  }


  if (_renderingParameters.billboardedNodes()) {
    GlyphsRenderer::instance()->setBillboardMode(true);
  }
  GlyphsRenderer::instance()->renderGlyphs(camera, light, NodeShape::Square, centers, sizes, colors, textures);
  GlyphsRenderer::instance()->setBillboardMode(false);


  glStencilMask(0xFF);
  glDepthMask(GL_TRUE);

}


void GlGraph::renderPointsNodesAndEdges(const Camera &camera, const std::vector<tlp::node> &pointsNodes, const std::vector<tlp::edge> &pointsEdges) {

  if (pointsNodes.empty() && pointsEdges.empty()) return;

  std::vector<float> pointsData;
  pointsData.reserve((pointsNodes.size()+pointsEdges.size()) * 8);

  for (size_t i = 0 ; i < pointsEdges.size() ; ++i) {
    Coord pointEdge = (_viewLayout->getNodeValue(_graph->source(pointsEdges[i])) + _viewLayout->getNodeValue(_graph->target(pointsEdges[i]))) / 2.f;
    if (_graphElementsPickingMode) {
      addTlpVecToVecFloat(Vec4f(pointEdge, 2.0f), pointsData);
      addColorToVecFloat(uintToColor(_graph->getRoot()->numberOfNodes() + pointsEdges[i].id+1), pointsData);
    } else if (_viewSelection->getEdgeValue(pointsEdges[i])) {
      addTlpVecToVecFloat(Vec4f(pointEdge, 4.0f), pointsData);
      addColorToVecFloat(_renderingParameters.selectionColor(), pointsData);
    } else {
      addTlpVecToVecFloat(Vec4f(pointEdge, 2.0f), pointsData);
      if (_viewBorderWidth->getEdgeValue(pointsEdges[i]) > 0) {
        addColorToVecFloat(_viewBorderColor->getEdgeValue(pointsEdges[i]), pointsData);
      } else {
        addColorToVecFloat(_viewColor->getEdgeValue(pointsEdges[i]), pointsData);
      }
    }
  }

  for (size_t i = 0 ; i < pointsNodes.size() ; ++i) {
    if (_graphElementsPickingMode) {
      addTlpVecToVecFloat(Vec4f(_viewLayout->getNodeValue(pointsNodes[i]), 3.0f), pointsData);
      addColorToVecFloat(uintToColor(pointsNodes[i].id+1), pointsData);
    } else if (_viewSelection->getNodeValue(pointsNodes[i])) {
      addTlpVecToVecFloat(Vec4f(_viewLayout->getNodeValue(pointsNodes[i]), 4.0f), pointsData);
      addColorToVecFloat(_renderingParameters.selectionColor(), pointsData);
    } else {
      addTlpVecToVecFloat(Vec4f(_viewLayout->getNodeValue(pointsNodes[i]), 3.0f), pointsData);
      if (_viewBorderWidth->getNodeValue(pointsNodes[i]) > 0) {
        addColorToVecFloat(_viewBorderColor->getNodeValue(pointsNodes[i]), pointsData);
      } else {
        addColorToVecFloat(_viewColor->getNodeValue(pointsNodes[i]), pointsData);
      }
    }
  }

#ifndef __EMSCRIPTEN__
  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

  _pointsDataBuffer->bind();
  _pointsDataBuffer->allocate(pointsData);
  _flatShader->activate();
  _flatShader->setUniformMat4Float("u_projectionMatrix", camera.projectionMatrix());
  _flatShader->setUniformMat4Float("u_modelviewMatrix", camera.modelviewMatrix());
  _flatShader->setUniformBool("u_textureActivated", false);
  _flatShader->setUniformBool("u_globalColor", false);
  _flatShader->setUniformBool("u_pointsRendering", true);
  _flatShader->setUniformBool("u_globalPointSize", false);
  _flatShader->setVertexAttribPointer("a_position", 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(0));
  _flatShader->setVertexAttribPointer("a_pointSize", 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(3*sizeof(float)));
  _flatShader->setVertexAttribPointer("a_color", 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(4*sizeof(float)));
  if (_renderingParameters.displayEdges()) {
    glStencilFunc(GL_LEQUAL, _renderingParameters.edgesStencil(), 0xFF);
    glDrawArrays(GL_POINTS, 0, pointsEdges.size());
  }
  if (_renderingParameters.displayNodes()) {
    glStencilFunc(GL_LEQUAL, _renderingParameters.nodesStencil(), 0xFF);
    glDrawArrays(GL_POINTS, pointsEdges.size(), pointsNodes.size());
  }
  _pointsDataBuffer->release();
}

void GlGraph::getEdgeExtremityData(edge e, bool srcGlyph, tlp::Coord &position, tlp::Size &size, tlp::Vec4f &rotationAxisAndAngle) {
  const vector<Vec3f> &eeData = srcGlyph ? _srcEdgeExtremitiesData[e] : _tgtEdgeExtremitiesData[e];
  Vec3f dir = eeData[1] - eeData[0];
  if (dir.norm() > 0) dir /= dir.norm();

  position = eeData[1] - eeData[2]/2.f * dir;
  size = eeData[2];

  Coord cross = dir^Coord(0,1,0);

  if (cross.norm() > 0) cross /= cross.norm();

  float rotationAngle = -acos(dir.dotProduct(Coord(0,1,0)));
  rotationAxisAndAngle = Vec4f(cross, rotationAngle);
}

void GlGraph::renderEdgeExtremities(const Camera &camera, const Light &light, const tlp::edge &e) {

  glStencilFunc(GL_LEQUAL, _renderingParameters.edgesStencil(), 0xFF);

  Coord center;
  Size size;
  Vec4f rotationAxisAndAngle;
  Color color;
  string texture;
  float borderWidth;
  Color borderColor;

  int srcShape = _viewSrcAnchorShape->getEdgeValue(e);
  int tgtShape = _viewTgtAnchorShape->getEdgeValue(e);

  if (srcShape == tlp::EdgeExtremityShape::FontAwesomeIcon) {
    std::string icon = _viewFontAwesomeIcon->getEdgeValue(e);
    srcShape = tlp::TulipFontAwesome::getFontAwesomeIconCodePoint(icon);
  }

  if (tgtShape == tlp::EdgeExtremityShape::FontAwesomeIcon) {
    std::string icon = _viewFontAwesomeIcon->getEdgeValue(e);
    tgtShape = tlp::TulipFontAwesome::getFontAwesomeIconCodePoint(icon);
  }

  if (srcShape != EdgeExtremityShape::None) {

    getEdgeExtremityData(e, true, center, size, rotationAxisAndAngle);

    color = _viewColor->getEdgeValue(e);
    borderColor = _viewBorderColor->getEdgeValue(e);
    if (_renderingParameters.interpolateEdgesColors()) {
      color = _viewColor->getNodeValue(_graph->source(e));
      borderColor = color;
    }

    texture = _viewTexture->getEdgeValue(e);

    if (!_graphElementsPickingMode) {
      if (_viewSelection->getEdgeValue(e)) {
        color = _renderingParameters.selectionColor();
        borderColor = _renderingParameters.selectionColor();
      }
    } else {
      tlp::Color pickColor = uintToColor(_graph->getRoot()->numberOfNodes()+e.id+1);
      color = pickColor;
      borderColor = pickColor;
      texture = "";
    }

    borderWidth = _viewBorderWidth->getEdgeValue(e);


    bool swapYZ = false;
    if (srcShape == EdgeExtremityShape::Cone || srcShape == EdgeExtremityShape::Cylinder) {
      swapYZ = true;
    }

    GlyphsRenderer::instance()->renderGlyph(camera, light, srcShape, center, size, color, texture, borderWidth, borderColor, rotationAxisAndAngle, _graphElementsPickingMode, swapYZ);

  }

  if (tgtShape != EdgeExtremityShape::None) {

    getEdgeExtremityData(e, false, center, size, rotationAxisAndAngle);

    color = _viewColor->getEdgeValue(e);
    borderColor = _viewBorderColor->getEdgeValue(e);
    if (_renderingParameters.interpolateEdgesColors()) {
      color = _viewColor->getNodeValue(_graph->target(e));
      borderColor = color;
    }

    texture = _viewTexture->getEdgeValue(e);

    if (!_graphElementsPickingMode) {
      if (_viewSelection->getEdgeValue(e)) {
        color =_renderingParameters.selectionColor();
        borderColor = _renderingParameters.selectionColor();
      }
    } else {
      tlp::Color pickColor = uintToColor(_graph->getRoot()->numberOfNodes()+e.id+1);
      color = pickColor;
      borderColor = pickColor;
      texture  = "";
    }

    borderWidth = _viewBorderWidth->getEdgeValue(e);

    bool swapYZ = false;
    if (tgtShape == EdgeExtremityShape::Cone || tgtShape == EdgeExtremityShape::Cylinder) {
      swapYZ = true;
    }

    GlyphsRenderer::instance()->renderGlyph(camera, light, tgtShape, center, size, color, texture, borderWidth, borderColor, rotationAxisAndAngle, _graphElementsPickingMode, swapYZ);

  }
}

void GlGraph::renderEdges(const Camera &camera, const Light &light, const std::vector<edge> &edges, bool lineMode, bool billboard) {

  if (!lineMode && billboard && !_graphElementsPickingMode) {
    TextureManager::instance()->addTextureFromFile("resources/cylinderTexture.png");
    TextureManager::instance()->bindTexture("resources/cylinderTexture.png");
  }

  glStencilFunc(GL_LEQUAL, _renderingParameters.edgesStencil(), 0xFF);

  std::vector<unsigned int> edgesLinesRenderingIndices;
  set<GlShaderProgram*> globalUniformsSet;

  for (size_t i = 0 ; i < edges.size() ; ++i) {
    edge e = edges[i];
    node src = _graph->source(e);
    node tgt = _graph->target(e);
    const Coord &srcCoord = _viewLayout->getNodeValue(src);
    const Coord &tgtCoord = _viewLayout->getNodeValue(tgt);
    Color srcColor = _viewColor->getEdgeValue(e);
    Color tgtColor = _viewColor->getEdgeValue(e);
    Color borderColor = _viewBorderColor->getEdgeValue(e);
    if (_renderingParameters.interpolateEdgesColors()) {
      srcColor = _viewColor->getNodeValue(src);
      tgtColor = _viewColor->getNodeValue(tgt);
    } else if (lineMode && _viewBorderWidth->getEdgeValue(e) > 0) {
      srcColor = tgtColor = borderColor;
    }

    if (_graphElementsPickingMode) {
      srcColor = tgtColor = borderColor = uintToColor(_graph->getRoot()->numberOfNodes()+e.id+1);
    } else if (_viewSelection->getEdgeValue(e)) {
      srcColor = tgtColor = borderColor = _renderingParameters.selectionColor();
    }

    int edgeShape = _viewShape->getEdgeValue(e);

    unsigned int nbCurvePoints = _edgePoints[e].size();
    unsigned int indicesOffset = _maxEdgePoints;
    bool bezierMode = (_edgePoints[e].size() > 2) && (edgeShape == tlp::EdgeShape::BezierCurve);
    bool bsplineMode = (_edgePoints[e].size() > 2) && (edgeShape == tlp::EdgeShape::CubicBSplineCurve);
    if (bsplineMode) {
      if (_edgePoints[e].size() < bSplineDegree + 1) {
        bsplineMode = false;
        bezierMode = true;
        edgeShape = tlp::EdgeShape::BezierCurve;
      }
    }
    bool catmullMode = (_edgePoints[e].size() > 2) && (edgeShape == tlp::EdgeShape::CatmullRomCurve);

    bool parametricCurveMode = bezierMode || bsplineMode || catmullMode;
    if (parametricCurveMode) {
      nbCurvePoints = nbCurveInterpolationPoints;
      indicesOffset = nbCurveInterpolationPoints;
    } else {
      edgeShape = tlp::EdgeShape::Polyline;
    }

    if (!lineMode || parametricCurveMode || _viewSelection->getEdgeValue(e)) {

      string edgeTexture = _viewTexture->getEdgeValue(e);
      Size edgeSize = ::getEdgeSize(_graph, e, _viewSize, &_renderingParameters);

      vector<Vec4f> edgePoints;
      float length = 0;
      if (lineMode) {
        edgePoints.push_back(Vec4f(srcCoord, length));
        for (size_t j = 1 ; j < _edgePoints[e].size() - 1; ++j) {
          if (j > 0) {
            if (!catmullMode) {
              length += _edgePoints[e][j].dist(edgePoints.back());
            } else {
              length += pow(_edgePoints[e][j].dist(edgePoints.back()), alpha);
            }
          }
          edgePoints.push_back(Vec4f(_edgePoints[e][j], length));
        }
        if (!catmullMode) {
          length += tgtCoord.dist(edgePoints.back());
        } else {
          length += pow(tgtCoord.dist(edgePoints.back()), alpha);
        }
        edgePoints.push_back(Vec4f(tgtCoord, length));
      } else {
        for (size_t j = 0 ; j < _edgePoints[e].size() ; ++j) {
          if (j > 0) {
            if (!catmullMode) {
              length += _edgePoints[e][j].dist(_edgePoints[e][j-1]);
            } else {
              length += pow(_edgePoints[e][j].dist(_edgePoints[e][j-1]), alpha);
            }
          }
          edgePoints.push_back(Vec4f(_edgePoints[e][j], length));
        }
      }

      GlShaderProgram *edgeShader = getEdgeShader(edgeShape);
      edgeShader->activate();
      if (globalUniformsSet.find(edgeShader) == globalUniformsSet.end()) {
        edgeShader->setUniformBool("u_lineMode", lineMode);
        edgeShader->setUniformBool("u_billboard", billboard);
        edgeShader->setUniformBool("u_billboardTex", billboard && !_graphElementsPickingMode);
        edgeShader->setUniformMat4Float("u_projectionMatrix", camera.projectionMatrix());
        edgeShader->setUniformMat4Float("u_modelviewMatrix", camera.modelviewMatrix());
        edgeShader->setUniformVec3Float("u_lookDir", camera.getCenter() - camera.getEyes());
        edgeShader->setUniformTextureSampler("u_texture0", 0);
        edgeShader->setUniformTextureSampler("u_texture1", 1);
        edgeShader->setUniformFloat("u_curveInterpolationStep", 1.0f / (nbCurveInterpolationPoints - 1));
        globalUniformsSet.insert(edgeShader);
      }

      edgeShader->setUniformVec4FloatArray("u_curvePoints", _edgePoints[e].size(), &edgePoints[0][0]);
      edgeShader->setUniformInt("u_nbCurvePoints", _edgePoints[e].size());
      edgeShader->setUniformFloat("u_startSize", edgeSize[0]);
      edgeShader->setUniformFloat("u_endSize", edgeSize[1]);
      edgeShader->setUniformColor("u_startColor", srcColor);
      edgeShader->setUniformColor("u_endColor", tgtColor);
      edgeShader->setUniformVec3Float("u_startN", srcCoord);
      edgeShader->setUniformVec3Float("u_endN", tgtCoord);
      edgeShader->setUniformBool("u_textureActivated", !lineMode && !edgeTexture.empty() && !_graphElementsPickingMode);
      edgeShader->setUniformBool("u_bezierMode", bezierMode);
      edgeShader->setUniformBool("u_bsplineMode", bsplineMode);
      edgeShader->setUniformBool("u_catmullMode", catmullMode);

      unsigned int nbKnots = edgePoints.size() + bSplineDegree + 1;
      float stepKnots = 1.0f / ((static_cast<float>(nbKnots) - 2.0f * (static_cast<float>(bSplineDegree) + 1.0f)) + 2.0f - 1.0f);

      edgeShader->setUniformFloat("u_stepKnots", stepKnots);

      TextureManager::instance()->addTextureFromFile(edgeTexture, true);
      TextureManager::instance()->bindTexture(edgeTexture, 1);

      if (!parametricCurveMode) {
        _edgeIndicesBuffer->bind();
        _edgeRenderingDataBuffer->bind();
      } else {
        _curveEdgeIndicesBuffer->bind();
        _curveEdgeRenderingDataBuffer->bind();
      }
      edgeShader->setVertexAttribPointer("a_position", 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), BUFFER_OFFSET(0));

      if (lineMode) {
        glLineWidth(2.0);
        glDrawElements(GL_LINE_STRIP, nbCurvePoints, GL_UNSIGNED_SHORT, BUFFER_OFFSET(indicesOffset*2*sizeof(unsigned short)));
      } else {
        glDrawElements(GL_TRIANGLE_STRIP, nbCurvePoints*2, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        if (_renderingParameters.displayEdgesExtremities()) {
          renderEdgeExtremities(camera, light, e);
        }
      }

      if (!lineMode && _viewBorderWidth->getEdgeValue(e) > 0) {
        TextureManager::instance()->unbindTexture(edgeTexture);

        if (!_renderingParameters.interpolateEdgesColors()) {
          srcColor = borderColor;
          tgtColor = borderColor;
        }

        edgeShader->setUniformBool("u_textureActivated", false);
        edgeShader->setUniformColor("u_startColor", srcColor);
        edgeShader->setUniformColor("u_endColor", tgtColor);

        glLineWidth(_viewBorderWidth->getEdgeValue(e));
        glDrawElements(GL_LINE_STRIP, nbCurvePoints, GL_UNSIGNED_SHORT, BUFFER_OFFSET(indicesOffset*2*sizeof(unsigned short)));
        glDrawElements(GL_LINE_STRIP, nbCurvePoints, GL_UNSIGNED_SHORT, BUFFER_OFFSET(indicesOffset*3*sizeof(unsigned short)));

      }
    } else {
      edgesLinesRenderingIndices.insert(edgesLinesRenderingIndices.end(), _edgeLineVerticesIndices[e].begin(), _edgeLineVerticesIndices[e].end());
    }
  }

  if (!edgesLinesRenderingIndices.empty()) {
    _edgeLineRenderingDataBuffer->bind();
    _edgeLineRenderingIndicesBuffer->bind();
    _edgeLineRenderingIndicesBuffer->allocate(edgesLinesRenderingIndices);
    _flatShader->activate();
    _flatShader->setUniformMat4Float("u_projectionMatrix", camera.projectionMatrix());
    _flatShader->setUniformMat4Float("u_modelviewMatrix", camera.modelviewMatrix());
    _flatShader->setUniformBool("u_textureActivated", false);
    _flatShader->setUniformBool("u_globalColor", false);
    _flatShader->setUniformBool("u_pointsRendering", false);
    _flatShader->setVertexAttribPointer("a_position", 3, GL_FLOAT, GL_FALSE, 15 * sizeof(float), BUFFER_OFFSET(0));
    if (_graphElementsPickingMode) {
      _flatShader->setVertexAttribPointer("a_color", 4, GL_FLOAT, GL_FALSE, 15 * sizeof(float), BUFFER_OFFSET(11*sizeof(float)));
    } else if (_renderingParameters.interpolateEdgesColors()) {
      _flatShader->setVertexAttribPointer("a_color", 4, GL_FLOAT, GL_FALSE, 15 * sizeof(float), BUFFER_OFFSET(7*sizeof(float)));
    } else {
      _flatShader->setVertexAttribPointer("a_color", 4, GL_FLOAT, GL_FALSE, 15 * sizeof(float), BUFFER_OFFSET(3*sizeof(float)));
    }
    glLineWidth(2.0);
    glDrawElements(GL_LINES, edgesLinesRenderingIndices.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));
    _flatShader->desactivate();

  }

  GlBuffer::release(GlBuffer::VertexBuffer);
  GlBuffer::release(GlBuffer::IndexBuffer);

  if (!lineMode && billboard && !_graphElementsPickingMode) {
    TextureManager::instance()->unbindTexture("resources/cylinderTexture.png");
  }

}

bool GlGraph::pickNodesAndEdges(const Camera &camera,
                                const int x, const int y,
                                const int width, const int height,
                                std::set<tlp::node> &selectedNodes,
                                std::set<tlp::edge> &selectedEdges) {

  selectedNodes.clear();
  selectedEdges.clear();
  tlp::Vec4i viewport = camera.getViewport();
  GlFrameBufferObject *fbo = new GlFrameBufferObject(viewport[2], viewport[3], GlFrameBufferObject::NoAttachment);
  fbo->bind();
  glViewport(0, 0, viewport[2], viewport[3]);
  unsigned int bufferSize = width*height*4;
  unsigned char *buffer = new unsigned char[bufferSize];
  bool done = false;
  setGraphElementsPickingMode(true);
  setSelectionViewport(Vec4i(x - viewport[2], viewport[3] - y, width, height));
  while (!done) {
    std::set<tlp::node> tmpNodeSet;
    std::set<tlp::edge> tmpEdgeSet;
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    GlEntity::draw(camera);
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    for (unsigned int i = 0 ; i < bufferSize ; i += 4) {
      tlp::Color color(buffer[i], buffer[i+1], buffer[i+2], buffer[i+3]);
      unsigned int id = colorToUint(color);
      if (id == 0) continue;
      id -= 1;
      if (id < _graph->getRoot()->numberOfNodes()) {
        tlp::node n = tlp::node(id);
        if (n.isValid()) {
          tmpNodeSet.insert(n);
        }
      } else {
        id -= _graph->getRoot()->numberOfNodes();
        tlp::edge e = tlp::edge(id);
        if (e.isValid()) {
          tmpEdgeSet.insert(e);
        }
      }
    }
    if (tmpNodeSet.empty() && tmpEdgeSet.empty()) {
      done = true;
    } else {
      selectedNodes.insert(tmpNodeSet.begin(), tmpNodeSet.end());
      selectedEdges.insert(tmpEdgeSet.begin(), tmpEdgeSet.end());
      _nodesToDiscard = selectedNodes;
      _edgesToDiscard = selectedEdges;
    }
  }
  setGraphElementsPickingMode(false);
  _nodesToDiscard.clear();
  _edgesToDiscard.clear();
  delete [] buffer;
  fbo->release();
  delete fbo;

  return !selectedNodes.empty() || !selectedEdges.empty();
}

bool GlGraph::pickNodeOrEdge(const Camera &camera,
                             const int x, const int y,
                             tlp::node &pickedNode,
                             tlp::edge &pickedEdge) {

  pickedNode = tlp::node();
  pickedEdge = tlp::edge();
  std::set<tlp::node> selectedNodes;
  std::set<tlp::edge> selectedEdges;
  bool ret = pickNodesAndEdges(camera, x-1, y-1, 3, 3, selectedNodes, selectedEdges);
  if (!selectedNodes.empty()) {
    pickedNode = *(selectedNodes.begin());
  } else if (!selectedEdges.empty()) {
    pickedEdge = *(selectedEdges.begin());
  }
  return ret;
}

Size GlGraph::getEdgeSize(edge e) {
  node src = _graph->source(e);
  node tgt = _graph->target(e);
  const Size &srcSize = _viewSize->getNodeValue(src);
  const Size &tgtSize = _viewSize->getNodeValue(tgt);
  const Size &edgeSize = _viewSize->getEdgeValue(e);

  float startSize = 0;
  float endSize = 0;
  if (!_renderingParameters.interpolateEdgesSizes()) {
    startSize = edgeSize[0] / 4.f;
    endSize = edgeSize[1] / 4.f;
    if (_renderingParameters.maxEdgesSizesToNodesSizes()) {
      float maxSrcSize = max(srcSize[0], srcSize[1]);
      float maxTgtSize = max(tgtSize[0], tgtSize[1]);
      startSize = min(startSize, maxSrcSize);
      endSize = min(endSize, maxTgtSize);
    }
  } else {
    if (srcSize[0] < srcSize[1]) {
      startSize = srcSize[0] / 16.f;
    } else {
      startSize = srcSize[1] / 16.f;
    }
    if (tgtSize[0] < tgtSize[1]) {
      endSize = tgtSize[0] / 16.f;
    } else {
      endSize = tgtSize[1] / 16.f;
    }
  }
  return Size(startSize, endSize);
}

void GlGraph::prepareEdgeData(tlp::edge e) {

  node src = _graph->source(e);
  node tgt = _graph->target(e);

  const Coord &srcCoord = _viewLayout->getNodeValue(src);
  const Coord &tgtCoord = _viewLayout->getNodeValue(tgt);
  const Size &srcSize = _viewSize->getNodeValue(src);
  const Size &tgtSize = _viewSize->getNodeValue(tgt);
  double srcRot = _viewRotation->getNodeValue(src);
  double tgtRot = _viewRotation->getNodeValue(tgt);
  int srcGlyphId = _viewShape->getNodeValue(src);
  int tgtGlyphId = _viewShape->getNodeValue(tgt);

  float maxSrcSize = max(srcSize[0], srcSize[1]);
  float maxTgtSize = max(tgtSize[0], tgtSize[1]);

  BoundingBox srcBB(srcCoord - srcSize/2.f, srcCoord + srcSize/2.f);
  BoundingBox tgtBB(tgtCoord - tgtSize/2.f, tgtCoord + tgtSize/2.f);

  const vector<Coord> &oribends = _viewLayout->getEdgeValue(e);
  vector<Coord> bends;
  for (size_t i = 0 ; i < oribends.size() ; ++i) {
    if (!srcBB.contains(oribends[i]) && !tgtBB.contains(oribends[i])) {
      bends.push_back(oribends[i]);
    }
  }

  Coord srcAnchor, tgtAnchor;
  vector<Coord> edgePoints;

  bool glyphsOk = false;
  bool srcGlyphOk = false;
  bool tgtGlyphOk = false;

  while (!glyphsOk) {

    srcAnchor = (bends.size() > 0) ? bends.front() : tgtCoord;
    srcAnchor = GlyphsManager::getGlyph(srcGlyphId)->getAnchor(srcCoord, srcAnchor, srcSize, srcRot);

    tgtAnchor = (bends.size() > 0) ? bends.back() : srcAnchor;
    tgtAnchor = GlyphsManager::getGlyph(tgtGlyphId)->getAnchor(tgtCoord, tgtAnchor, tgtSize, tgtRot);

    edgePoints.clear();
    edgePoints.push_back(srcAnchor);
    edgePoints.insert(edgePoints.end(), bends.begin(), bends.end());
    edgePoints.push_back(tgtAnchor);

    vector<Vec3f> eeData;
    eeData.reserve(3);
    if (_renderingParameters.displayEdgesExtremities() && _viewSrcAnchorShape->getEdgeValue(e) != EdgeExtremityShape::None) {

      Coord lineAnchor = bends.size() > 0 ? bends.front() : tgtAnchor;
      eeData.push_back(lineAnchor);
      eeData.push_back(srcAnchor);
      if (_renderingParameters.interpolateEdgesSizes()) {
        eeData.push_back(Vec3f(srcSize[0] / 4.f, srcSize[0] / 4.f, srcSize[0] / 4.f));
      } else {
        Size srcAnchorSize = _viewSrcAnchorSize->getEdgeValue(e);
        if (_renderingParameters.maxEdgesSizesToNodesSizes()) {
          srcAnchorSize[0] = min(srcAnchorSize[0], maxSrcSize);
          srcAnchorSize[1] = min(srcAnchorSize[1], maxSrcSize);
          srcAnchorSize[2] = min(srcAnchorSize[2], maxSrcSize);
        }
        eeData.push_back(srcAnchorSize);
      }
      _srcEdgeExtremitiesData[e] = eeData;
      lineAnchor -= srcAnchor;
      float nrm = lineAnchor.norm();
      if (nrm > 0.00000001f) {
        lineAnchor /= nrm;
        lineAnchor *= eeData.back()[0];
        lineAnchor += srcAnchor;
      }
      edgePoints[0] = lineAnchor;

      if (!bends.empty()) {
        Coord glyphPos;
        Size glyphSize;
        Vec4f glyphRot;

        getEdgeExtremityData(e, true, glyphPos, glyphSize, glyphRot);
        BoundingBox glyphBB(glyphPos-glyphSize/2.f, glyphPos+glyphSize/2.f);
        if (glyphBB.contains(bends.front())) {
          while (glyphBB.contains(bends.front())) {
            bends.erase(bends.begin());
            if (bends.empty()) break;
          }
        } else {
          srcGlyphOk = true;
        }
      } else {
        srcGlyphOk = true;
      }

    } else {
      srcGlyphOk = true;
    }

    eeData.clear();

    if (_renderingParameters.displayEdgesExtremities() && _viewTgtAnchorShape->getEdgeValue(e) != EdgeExtremityShape::None) {

      Coord lineAnchor = bends.size() > 0 ? bends.back() : srcAnchor;
      eeData.push_back(lineAnchor);
      eeData.push_back(tgtAnchor);
      if (_renderingParameters.interpolateEdgesSizes()) {
        eeData.push_back(Vec3f(tgtSize[0] / 4.f, tgtSize[0] / 4.f, tgtSize[0] / 4.f));
      } else {
        Size tgtAnchorSize = _viewTgtAnchorSize->getEdgeValue(e);
        if (_renderingParameters.maxEdgesSizesToNodesSizes()) {
          tgtAnchorSize[0] = min(tgtAnchorSize[0], maxTgtSize);
          tgtAnchorSize[1] = min(tgtAnchorSize[1], maxTgtSize);
          tgtAnchorSize[2] = min(tgtAnchorSize[2], maxTgtSize);
        }
        eeData.push_back(tgtAnchorSize);
      }
      _tgtEdgeExtremitiesData[e] = eeData;
      lineAnchor -= tgtAnchor;
      float nrm = lineAnchor.norm();
      if (nrm > 0.00000001f) {
        lineAnchor /= nrm;
        lineAnchor *= eeData.back()[0];
        lineAnchor += tgtAnchor;
      }
      edgePoints[edgePoints.size()-1] = lineAnchor;

      if (!bends.empty()) {
        Coord glyphPos;
        Size glyphSize;
        Vec4f glyphRot;

        getEdgeExtremityData(e, false, glyphPos, glyphSize, glyphRot);
        BoundingBox glyphBB(glyphPos-glyphSize/2.f, glyphPos+glyphSize/2.f);
        if (glyphBB.contains(bends.back())) {
          while (glyphBB.contains(bends.back())) {
            bends.erase(bends.end());
            if (bends.empty()) break;
          }
        } else {
          tgtGlyphOk = true;
        }
      } else {
        tgtGlyphOk = true;
      }
    } else {
      tgtGlyphOk = true;
    }

    glyphsOk = srcGlyphOk && tgtGlyphOk;

  }

  _edgePoints[e] = edgePoints;
  _edgeAnchors[e] = make_pair(srcAnchor, tgtAnchor);

  _maxEdgePoints = std::max(_maxEdgePoints, edgePoints.size());

}

void GlGraph::treatEvent(const tlp::Event &message) {
  if (message.type() == Event::TLP_DELETE && dynamic_cast<tlp::Graph *>(message.sender())) {
    _graph = NULL;
    return;
  }
  if (message.type() != Event::TLP_MODIFICATION) return;
  node n;
  edge e;
  const GraphEvent *gEvt = dynamic_cast<const GraphEvent *>(&message);
  const PropertyEvent *pEvt = dynamic_cast<const PropertyEvent *>(&message);
  const GlGraphRenderingParametersEvent *rpEvt = dynamic_cast<const GlGraphRenderingParametersEvent *>(&message);
  if (gEvt) {
    if (gEvt->getType() == GraphEvent::TLP_ADD_NODE) {
      _nodesToUpdate.insert(gEvt->getNode());
    } else if (gEvt->getType() == GraphEvent::TLP_ADD_NODES) {
      const std::vector<node> nodes = gEvt->getNodes();
      for (size_t i = 0 ; i < nodes.size() ; ++i) {
        _nodesToUpdate.insert(nodes[i]);
      }
    } else if (gEvt->getType() == GraphEvent::TLP_DEL_NODE) {
      _labelsRenderer->removeNodeLabel(_graph, gEvt->getNode());
      _updateQuadTree = true;
      notifyModified();
    } else if (gEvt->getType() == GraphEvent::TLP_ADD_EDGE ||
               gEvt->getType() == GraphEvent::TLP_AFTER_SET_ENDS ||
               gEvt->getType() == GraphEvent::TLP_REVERSE_EDGE) {
      _edgesToUpdate.insert(gEvt->getEdge());
    } else if (gEvt->getType() == GraphEvent::TLP_ADD_EDGES) {
      const std::vector<edge> edges = gEvt->getEdges();
      for (size_t i = 0 ; i < edges.size() ; ++i) {
        _edgesToUpdate.insert(edges[i]);
      }
    } else if (gEvt->getType() == GraphEvent::TLP_DEL_EDGE) {
      _updateQuadTree = true;
      notifyModified();
    }
  }
  else if (pEvt && (pEvt->getProperty() == _viewLayout || pEvt->getProperty() == _viewSize || pEvt->getProperty() == _viewColor)) {
    if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_NODE_VALUE && _graph->isElement(pEvt->getNode())) {
      forEach(e, _graph->getInOutEdges(pEvt->getNode())) {
        _edgesToUpdate.insert(e);
      }
    }
    if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_ALL_NODE_VALUE) {
      forEach(e, _graph->getEdges()) {
        _edgesToUpdate.insert(e);
      }
    }
    if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_EDGE_VALUE &&
        (pEvt->getProperty() == _viewLayout || pEvt->getProperty() == _viewColor) &&
        _graph->isElement(pEvt->getEdge())) {
      _edgesToUpdate.insert(pEvt->getEdge());
    }
    if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_ALL_EDGE_VALUE &&
        (pEvt->getProperty() == _viewLayout || pEvt->getProperty() == _viewColor)) {
      forEach(e, _graph->getEdges()) {
        _edgesToUpdate.insert(e);
      }
    }
  } else if (pEvt && pEvt->getProperty() == _viewLabel) {
    if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_NODE_VALUE && _graph->isElement(pEvt->getNode())) {
      _nodesToUpdate.insert(pEvt->getNode());
    } else if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_ALL_NODE_VALUE) {
      forEach(n, _graph->getNodes()) {
        _nodesToUpdate.insert(n);
      }
    }
  } else if (pEvt && (pEvt->getProperty() == _viewShape || pEvt->getProperty() == _viewBorderWidth || pEvt->getProperty() == _viewBorderColor)) {
    if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_ALL_EDGE_VALUE) {
      forEach(e, _graph->getEdges()) {
        _edgesToUpdate.insert(e);
      }
    }
    if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_EDGE_VALUE && _graph->isElement(pEvt->getEdge())) {
      _edgesToUpdate.insert(pEvt->getEdge());
    }
    if (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_NODE_VALUE || pEvt->getType() == PropertyEvent::TLP_AFTER_SET_ALL_NODE_VALUE) {
      notifyModified();
    }
  } else if (pEvt && (pEvt->getProperty() == _viewGlow || pEvt->getProperty() == _viewFontAwesomeIcon || pEvt->getProperty() == _viewLabelColor) &&
             (pEvt->getType() == PropertyEvent::TLP_AFTER_SET_ALL_NODE_VALUE || pEvt->getType() == PropertyEvent::TLP_AFTER_SET_NODE_VALUE)) {

    notifyModified();
  } else if (rpEvt && (rpEvt->getType() == GlGraphRenderingParametersEvent::DISPLAY_EDGES_EXTREMITIES_TOGGLED)) {
    forEach(e, _graph->getEdges()) {
      _edgesToUpdate.insert(e);
    }
  } else if (rpEvt && rpEvt->getType() == GlGraphRenderingParametersEvent::RENDERING_PARAMETERS_MODIFIED) {
    notifyModified();
  }
}

void GlGraph::treatEvents(const std::vector<tlp::Event> &) {

  if (!_edgesToUpdate.empty() || !_nodesToUpdate.empty()) {

    set<node>::iterator itN;
    for (itN = _nodesToUpdate.begin() ; itN != _nodesToUpdate.end() ; ++itN) {
      prepareNodeLabelData(*itN);
    }

    set<edge>::iterator itE;
    for (itE = _edgesToUpdate.begin() ; itE != _edgesToUpdate.end() ; ++itE) {
      prepareEdgeData(*itE);
    }
    if (!_edgesToUpdate.empty()) {
      endEdgesData();
    }

    _edgesToUpdate.clear();
    _nodesToUpdate.clear();

    computeGraphBoundingBox();

    _updateQuadTree = true;

    notifyModified();

  }

}
