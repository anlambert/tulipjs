#include <GL/glew.h>

#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <ctime>

#include <tulip/Graph.h>
#include <tulip/GraphAbstract.h>
#include <tulip/TlpTools.h>
#include <tulip/PluginLibraryLoader.h>
#include <tulip/PropertyTypes.h>
#include <tulip/LayoutProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/ColorProperty.h>
#include <tulip/BooleanProperty.h>
#include <tulip/IntegerProperty.h>
#include <tulip/DoubleProperty.h>
#include <tulip/StringProperty.h>
#include <tulip/PluginLister.h>
#include <tulip/ExportModule.h>
#include <tulip/SimplePluginProgress.h>
#include <tulip/Color.h>
#include <tulip/PropertyTypes.h>
#include <tulip/ColorScale.h>

#include "GlGraph.h"
#include "Camera.h"
#include "Light.h"
#include "GlRect2D.h"

#include "ZoomAndPanInteractor.h"
#include "RectangleZoomInteractor.h"
#include "SelectionInteractor.h"
#include "SelectionModifierInteractor.h"
#include "NeighborhoodInteractor.h"
#include "LassoSelectionInteractor.h"

#include "Utils.h"
#include "TextureManager.h"
#include "LabelsRenderer.h"
#include "GlScene.h"
#include "GlLayer.h"
#include "GlLODCalculator.h"
#include "ConcaveHullBuilder.h"
#include "GlConcavePolygon.h"
#include "ShaderManager.h"
#include "GlShaderProgram.h"
#include "glyphs/GlyphsRenderer.h"

#include <GL/glut.h>

#include <QFileDialog>
#include <QApplication>

static int CURRENT_WIDTH = 1024;
static int CURRENT_HEIGHT = 768;

static tlp::Color backgroundColor(255,255,255);
static GlScene * glScene;
static GlGraph * glGraph;
static GlLayer * hullsLayer;
static tlp::Graph * graph;

static ZoomAndPanInteractor zoomAndPanInteractor;
static RectangleZoomInteractor rectangleZoomInteractor;
static SelectionInteractor selectionInteractor;
static SelectionModifierInteractor selectionModifierInteractor;
static NeighborhoodInteractor neighborhoodInteractor;
static LassoSelectionInteractor lassoSelectionInteractor;
static GlSceneInteractor *currentInteractor = &zoomAndPanInteractor;

const std::string defaultGraphFilePath = "data/programming_language_network.tlp.gz";

static void (*animFunc)(void *);
static void *animFuncParam;

static void animationFunc(int) {
  animFunc(animFuncParam);
}

void timerFunc(unsigned int msecs, void (*func)(void *value), void *value) {
  animFunc = func;
  animFuncParam = value;
  glutTimerFunc(msecs, animationFunc, 0);
}

void activateNavigationInteractor() {
  currentInteractor = &zoomAndPanInteractor;
}

void activateZoomInteractor() {
  currentInteractor = &rectangleZoomInteractor;
}

void activateSelectionInteractor() {
  currentInteractor = &selectionInteractor;
}

void activateSelectionModifierInteractor() {
  currentInteractor = &selectionModifierInteractor;
}

void activateLassoSelectionInteractor() {
  currentInteractor = &lassoSelectionInteractor;
}

void activateNeighborhoodInteractor() {
  currentInteractor = &neighborhoodInteractor;
}

static void glutReshapeCallback(int width, int height) {
  CURRENT_WIDTH = width;
  CURRENT_HEIGHT = height;
  tlp::Vec4i viewport(0, 0, width, height);
  glScene->setViewport(viewport);
  glutPostRedisplay();
}

static void glutDrawCallback(void) {
  glScene->draw();
  currentInteractor->draw();
  glutSwapBuffers();
  checkOpenGLError();
}

static void mouseCallback(int button, int state, int x, int y) {
  MouseButton buttonVal = LEFT_BUTTON;
  MouseButtonState buttonStateVal = DOWN;
  if (state == GLUT_DOWN) {
    buttonStateVal = DOWN;
  } else {
    buttonStateVal = UP;
  }

  if (button == GLUT_LEFT_BUTTON) {
    buttonVal = LEFT_BUTTON;
  } else if (button == GLUT_RIGHT_BUTTON) {
    buttonVal = RIGHT_BUTTON;
  } else if (button == 3) {
    buttonVal = WHEEL;
    buttonStateVal = UP;
  } else if (button == 4) {
    buttonVal = WHEEL;
    buttonStateVal = DOWN;
  }
  currentInteractor->mouseCallback(buttonVal, buttonStateVal, x, y, glutGetModifiers());
}

static tlp::Graph* loadGraph(const char *filename) {
  tlp::Graph *g = NULL;
  if (filename) {
    g = tlp::loadGraph(filename);
  }
  if (g) {
    std::cout << "Graph " << filename << std::endl;
    std::cout << "- Number nodes : " << g->numberOfNodes() << std::endl;
    std::cout << "- Number edges : " << g->numberOfEdges() << std::endl;
    tlp::StringProperty *viewTexture = g->getProperty<tlp::StringProperty>("viewTexture");
    tlp::node n;
    forEach(n, g->getNodes()) {
      std::string texture = viewTexture->getNodeValue(n);
      if (!texture.empty()) {
        TextureManager::instance()->addTextureInAtlasFromFile(texture);
      }
    }
  }
  return g;
}

static void keyboardCallback(const unsigned char key, const int , const int ) {
  if (key == '1') {
    activateNavigationInteractor();
    glutPostRedisplay();
  } else if (key == '2') {
    activateZoomInteractor();
    glutPostRedisplay();
  } else if (key == '3') {
    activateSelectionInteractor();
    glutPostRedisplay();
  } else if (key == '4') {
    activateLassoSelectionInteractor();
    glutPostRedisplay();
  } else if (key == '5') {
    activateNeighborhoodInteractor();
    glutPostRedisplay();
  } else if (key == '6') {
    activateSelectionModifierInteractor();
    glutPostRedisplay();
  }
  else if (key == 'o') {
    QString graphFile = QFileDialog::getOpenFileName(0, "open graph file", ".", "Tulip graph (*.tlp *.tlp.gz)");
    if (!graphFile.isEmpty()) {
      tlp::Graph *g = loadGraph(graphFile.toStdString().c_str());
      if (g) {
        delete graph;
        graph = g;
        glGraph->setGraph(graph);
        glScene->centerScene();
        glutPostRedisplay();
      }
    }
  } else {
    std::string keyStr;
    keyStr.push_back(key);
    currentInteractor->keyboardCallback(keyStr, glutGetModifiers());
  }
}

static void specialKeyboardCallback(int special, int , int ) {

  std::string specialKey;

  switch (special) {

  case GLUT_KEY_F1: {
    specialKey = "F1";
    break;
  }
  case GLUT_KEY_F2: {
    specialKey = "F2";
    break;
  }
  case GLUT_KEY_F3: {
    specialKey = "F3";
    break;
  }
  case GLUT_KEY_F4: {
    specialKey = "F4";
    break;
  }
  case GLUT_KEY_F5: {
    specialKey = "F5";
    break;
  }
  case GLUT_KEY_F6: {
    specialKey = "F6";
    break;
  }
  case GLUT_KEY_F7: {
    specialKey = "F7";
    break;
  }
  case GLUT_KEY_F8: {
    specialKey = "F8";
    break;
  }
  case GLUT_KEY_F9: {
    specialKey = "F9";
    break;
  }
  case GLUT_KEY_F10: {
    specialKey = "F10";
    break;
  }
  case GLUT_KEY_F11: {
    specialKey = "F11";
    break;
  }
  case GLUT_KEY_F12: {
    specialKey = "F12";
    break;
  }
  case GLUT_KEY_LEFT: {
    specialKey = "Left";
    break;
  }
  case GLUT_KEY_UP: {
    specialKey = "Up";
    break;
  }
  case GLUT_KEY_RIGHT: {
    specialKey = "Right";
    break;
  }
  case GLUT_KEY_DOWN: {
    specialKey = "Down";
    break;
  }
  case GLUT_KEY_PAGE_UP: {
    specialKey = "PageUp";
    break;
  }
  case GLUT_KEY_PAGE_DOWN: {
    specialKey = "PageDown";
    break;
  }
  case GLUT_KEY_HOME: {
    specialKey = "Home";
    break;
  }
  case GLUT_KEY_END: {
    specialKey = "End";
    break;
  }
  default: {
    specialKey = "Insert";
    break;
  }

  }
  currentInteractor->keyboardCallback(specialKey, glutGetModifiers());
}

static void mouseMoveCallback(int x, int y) {
  currentInteractor->mouseMoveCallback(x, y, glutGetModifiers());
}

class GlDrawObserver : public tlp::Observable {

public:

  void treatEvent(const tlp::Event &event) {
    const GlSceneEvent *glSceneEvent = dynamic_cast<const GlSceneEvent*>(&event);

    if (glSceneEvent) {
      if(glSceneEvent->getType() == GlSceneEvent::DRAW_REQUEST) {
        glutPostRedisplay();
      }
    }
  }

};

static GlDrawObserver glDrawObserver;

//==============================================================
int  main(int argc, char *argv[]) {

  setApplicationDirPath(argv[0]);
  QApplication app(argc, argv);

  tlp::initTulipLib();
  tlp::PluginLibraryLoader::loadPlugins();

  glutInit(&argc, argv);
  glutInitWindowSize(CURRENT_WIDTH, CURRENT_HEIGHT);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);
  if ((glutCreateWindow("Tulip Glut Viewer")) == GL_FALSE) {
    std::cerr << "Unable to create an OpenGl Glut window" << std::endl;
    exit(EXIT_FAILURE);
  }

  glutDisplayFunc(glutDrawCallback);
  glutReshapeFunc(glutReshapeCallback);
  glutSpecialFunc(specialKeyboardCallback);
  glutMouseFunc(mouseCallback);
  glutKeyboardFunc(keyboardCallback);
  glutMotionFunc(mouseMoveCallback);

  glewInit();

  tlp::Vec4i viewport(0, 0, CURRENT_WIDTH, CURRENT_HEIGHT);

  glScene = new GlScene();
  glScene->addListener(&glDrawObserver);

  glScene->getMainLayer()->getLight()->setDirectionnalLight(true);
  glScene->setViewport(viewport);

  glGraph = new GlGraph();
  glScene->getMainLayer()->addGlEntity(glGraph, "graph");

  hullsLayer = glScene->createLayerAfter("hulls", "Main");

  zoomAndPanInteractor.setScene(glScene);
  selectionInteractor.setScene(glScene);
  rectangleZoomInteractor.setScene(glScene);
  selectionModifierInteractor.setScene(glScene);
  neighborhoodInteractor.setScene(glScene);
  lassoSelectionInteractor.setScene(glScene);

  if (argc > 1) {
    graph = loadGraph(argv[1]);
  } else {
    graph = loadGraph(defaultGraphFilePath.c_str());
  }
  if (graph) {
    glGraph->setGraph(graph);
    glScene->centerScene();
    glutMainLoop();
  }


  return 0;
}
