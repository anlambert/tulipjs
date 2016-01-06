CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(POLICY CMP0020)
  CMAKE_POLICY(SET CMP0020 NEW)
ENDIF(POLICY CMP0020)

IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

SET(BUILD_TYPE release)
IF(NOT CMAKE_BUILD_TYPE MATCHES "[Rr][Ee][Ll][Ee][Aa][Ss][Ee]")
  SET(BUILD_TYPE debug)
ENDIF(NOT CMAKE_BUILD_TYPE MATCHES "[Rr][Ee][Ll][Ee][Aa][Ss][Ee]")

SET(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})

FIND_PACKAGE(TULIP REQUIRED)

INCLUDE(${TULIP_USE_FILE})

SET_COMPILER_OPTIONS()

EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/ ${CMAKE_BINARY_DIR}/resources)
EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data/ ${CMAKE_BINARY_DIR}/data)

IF(NOT EMSCRIPTEN)

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(SDL REQUIRED)
FIND_PACKAGE(SDL_image REQUIRED)
FIND_PACKAGE(QtX REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
FIND_PACKAGE(Freetype REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)

EXEC_PROGRAM(uname OUTPUT_VARIABLE UNAME_OUT)
STRING(COMPARE EQUAL "${UNAME_OUT}" "Linux" LINUX)

IF(LINUX)
  ADD_SUBDIRECTORY(thirdparty)
ELSE(LINUX)
  FIND_PACKAGE(GLUT REQUIRED)
ENDIF(LINUX)

FIND_PACKAGE(Threads)
OPTION(ENABLE_OPENMP "Do you want to enable OpenMP?" ON)
IF(ENABLE_OPENMP)
    FIND_PACKAGE(OpenMP)
    IF(OPENMP_FOUND)
        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OpenMP_CXX_FLAGS}")
        SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${OpenMP_CXX_FLAGS}")
        SET(OPENMP_CXX_FLAGS "${OpenMP_CXX_FLAGS}")
        IF(WIN32)
            IF(MSVC)
                SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /openmp")
                SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /openmp")
                SET(OPENMP_CXX_FLAGS "/openmp")
            ELSE()
                SET(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lgomp ${CMAKE_THREAD_LIBS_INIT}")
                SET(OPENMP_LIBRARIES "-lgomp -lpthread")
            ENDIF()
        ENDIF()
    ELSE(OPENMP_FOUND)
        IF(WIN32)
            STRING(COMPARE NOTEQUAL "${OpenMP_C_FLAGS}" "" OMP_CFLAGS)
            IF(OMP_CFLAGS)
                # Force setting OpenMP flags on Windows plateforms
                SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OpenMP_C_FLAGS}")
                SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${OpenMP_C_FLAGS}")
                SET(OPENMP_CXX_FLAGS "${OpenMP_C_FLAGS}")
                IF(NOT MSVC)
                    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OpenMP_C_FLAGS}")
                    SET(OPENMP_LINKER_FLAGS "${OpenMP_C_FLAGS}")
                    SET(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lgomp -lpthread")
                    SET(OPENMP_LIBRARIES "-lgomp -lpthread")
                ENDIF(NOT MSVC)
                SET(OPENMP_FOUND TRUE)
            ELSE(OMP_CFLAGS)
                MESSAGE("OpenMP not found: multithreaded code will not be used")
            ENDIF(OMP_CFLAGS)
        ELSE(WIN32)
            MESSAGE("OpenMP not found: multithreaded code will not be used")
        ENDIF(WIN32)
        ## ========================================================
        ## Enable debugging of OpenMP
        ## ========================================================
        IF(CMAKE_DEBUG_MODE AND NOT CHECK_MEMORY)
            SET(DEBUG_OPENMP OFF CACHE BOOL "Do you want to enable OpenMP even in Debug mode ? [OFF|ON]")
            IF(DEBUG_OPENMP)
                SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${OPENMP_CXX_FLAGS}")
                SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OPENMP_LINKER_FLAGS}")
                SET(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ${OPENMP_LIBRARIES}")
            ENDIF(DEBUG_OPENMP)
        ENDIF(CMAKE_DEBUG_MODE AND NOT CHECK_MEMORY)
    ENDIF(OPENMP_FOUND)
ENDIF(ENABLE_OPENMP)

ENDIF(NOT EMSCRIPTEN)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

SET(GRAPH_RENDERER_SRCS
    src/tulip-ogles/GlEntity.cpp
    src/tulip-ogles/GlLayer.cpp
    src/tulip-ogles/GlScene.cpp
    src/tulip-ogles/GlGraph.cpp
    src/tulip-ogles/Camera.cpp
    src/tulip-ogles/Light.cpp
    src/tulip-ogles/Utils.cpp
    src/tulip-ogles/GlBuffer.cpp
    src/tulip-ogles/GlShaderProgram.cpp
    src/tulip-ogles/GlFrameBufferObject.cpp
    src/tulip-ogles/ShaderManager.cpp
    src/tulip-ogles/LabelsRenderer.cpp
    src/tulip-ogles/GlConvexPolygon.cpp
    src/tulip-ogles/GlConcavePolygon.cpp
    src/tulip-ogles/ConcaveHullBuilder.cpp
    src/tulip-ogles/GlRect2D.cpp
    src/tulip-ogles/GlComposite.cpp
    src/tulip-ogles/GlCPULODCalculator.cpp
    src/tulip-ogles/GlQuadTreeLODCalculator.cpp
    src/tulip-ogles/ZoomAndPanAnimation.cpp
    src/tulip-ogles/GlBoundingBoxSceneVisitor.cpp
    src/tulip-ogles/GlLODSceneVisitor.cpp
    src/tulip-ogles/GlGlyphEntity.cpp
    src/interactors/ZoomAndPanInteractor.cpp
    src/interactors/RectangleZoomInteractor.cpp
    src/interactors/SelectionInteractor.cpp
    src/interactors/SelectionModifierInteractor.cpp
    src/interactors/NeighborhoodInteractor.cpp
    src/interactors/LassoSelectionInteractor.cpp
    src/interactors/FisheyeInteractor.cpp
    src/tulip-ogles/glyphs/Glyph.cpp
    src/tulip-ogles/glyphs/SquareGlyph.cpp
    src/tulip-ogles/glyphs/TriangleGlyph.cpp
    src/tulip-ogles/glyphs/CircleGlyph.cpp
    src/tulip-ogles/glyphs/PentagonGlyph.cpp
    src/tulip-ogles/glyphs/HexagonGlyph.cpp
    src/tulip-ogles/glyphs/RoundedBoxGlyph.cpp
    src/tulip-ogles/glyphs/CrossGlyph.cpp
    src/tulip-ogles/glyphs/ConeGlyph.cpp
    src/tulip-ogles/glyphs/DiamondGlyph.cpp
    src/tulip-ogles/glyphs/StarGlyph.cpp
    src/tulip-ogles/glyphs/SphereGlyph.cpp
    src/tulip-ogles/glyphs/CubeGlyph.cpp
    src/tulip-ogles/glyphs/RingGlyph.cpp
    src/tulip-ogles/glyphs/CylinderGlyph.cpp
    src/tulip-ogles/glyphs/CubeOutlinedGlyph.cpp
    src/tulip-ogles/glyphs/CubeOutlinedTransparentGlyph.cpp
    src/tulip-ogles/glyphs/FontAwesomeGlyph.cpp
    src/tulip-ogles/glyphs/GlyphsManager.cpp
    src/tulip-ogles/glyphs/GlyphsRenderer.cpp
    src/tulip-ogles/TextureManager.cpp
    src/tulip-ogles/GlGraphRenderingParameters.cpp
    src/tulip-ogles/GlProgressBar.cpp
    src/tulip-ogles/NanoVGManager.cpp
    src/utils/TimeMeasurer.cpp
    thirdparty/freetype-gl++/TextureAtlas.cpp
    thirdparty/freetype-gl++/TextureFont.cpp
    thirdparty/freetype-gl++/FTLibrary.cpp
    thirdparty/freetype-gl++/edtaa3func.cpp
    thirdparty/clipper/cpp/clipper.cpp
    thirdparty/libtess2/Source/bucketalloc.c
    thirdparty/libtess2/Source/dict.c
    thirdparty/libtess2/Source/geom.c
    thirdparty/libtess2/Source/mesh.c
    thirdparty/libtess2/Source/priorityq.c
    thirdparty/libtess2/Source/sweep.c
    thirdparty/libtess2/Source/tess.c
    thirdparty/ftgl/FTVectoriser.cpp
    thirdparty/ftgl/FTContour.cpp
    thirdparty/ftgl/FTPoint.cpp
    thirdparty/nanovg/nanovg.c
)

INCLUDE_DIRECTORIES(${TULIP_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/tulip-ogles ${CMAKE_SOURCE_DIR}/src/utils ${CMAKE_SOURCE_DIR}/src/interactors)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdparty/freetype-gl++ ${CMAKE_SOURCE_DIR}/thirdparty/utf8-cpp ${CMAKE_SOURCE_DIR}/thirdparty/clipper/cpp)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdparty/libtess2/Include ${CMAKE_SOURCE_DIR}/thirdparty/ftgl ${CMAKE_SOURCE_DIR}/thirdparty/nanovg)

IF(NOT EMSCRIPTEN)

SET(GRAPH_RENDERER_SRCS
    ${GRAPH_RENDERER_SRCS}
    src/glut/glut_main.cpp
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

IF(USE_QT5)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
ENDIF(USE_QT5)

IF(LINUX)
  SET(GLUT_INCLUDES ${CMAKE_SOURCE_DIR}/thirdparty/freeglut/include/)
  SET(GLUT_LIB freeglut)
ELSE(LINUX)
  SET(GLUT_INCLUDES ${GLU_INCLUDE_DIR})
  SET(GLUT_LIB ${GLUT_LIBRARIES})
ENDIF(LINUX)

INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${SDL_IMAGE_INCLUDE_DIRS} ${GLUT_INCLUDES})
ADD_EXECUTABLE(graph_renderer ${GRAPH_RENDERER_SRCS})

TARGET_LINK_LIBRARIES(graph_renderer ${TULIP_CORE_LIBRARY} ${ZLIB_LIBRARY} ${OPENGL_LIBRARY} ${GLEW_LIBRARY} ${FREETYPE_LIBRARY} gomp)
TARGET_LINK_LIBRARIES(graph_renderer  ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARIES} ${QT_LIBRARIES} ${GLUT_LIB})

#INSTALL(CODE "INCLUDE(BundleUtilities)
#              FIXUP_BUNDLE(\"${CMAKE_CURRENT_BINARY_DIR}/graph_renderer.exe\"   \"\"   \"${TULIP_DIR}/bin\")")

ELSE(NOT EMSCRIPTEN)

FIND_PACKAGE(PythonInterp REQUIRED)

EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} embuilder.py build zlib WORKING_DIRECTORY ${EMSCRIPTEN_ROOT_PATH})
EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} embuilder.py build freetype WORKING_DIRECTORY ${EMSCRIPTEN_ROOT_PATH})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_ZLIB=1 -s USE_FREETYPE=1")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_ZLIB=1 -s USE_FREETYPE=1")

SET(TULIPJS_CORE_SRCS
    src/emscripten/TulipBindings.cpp
    src/emscripten/main.cpp
)

SET(TULIPJS_SRCS
    ${GRAPH_RENDERER_SRCS}
    src/emscripten/TulipHtml5.cpp
    src/emscripten/TulipBindings.cpp
    src/emscripten/main.cpp
)

SET(CMAKE_EXECUTABLE_SUFFIX "")

FILE(GLOB_RECURSE TULIP_PLUGINS_LIBRARIES "${TULIP_PLUGINS_DIR}/*.so")

ADD_EXECUTABLE(tulip-node.js ${TULIPJS_CORE_SRCS})

TARGET_LINK_LIBRARIES(tulip-node.js ${TULIP_CORE_LIBRARY} ${TULIP_OGDF_LIBRARY} ${TULIP_PLUGINS_LIBRARIES})

SET_TARGET_PROPERTIES(tulip-node.js PROPERTIES LINK_FLAGS "--bind -s USE_ZLIB=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=\\\"tulipjs\\\" --memory-init-file 0")

EM_LINK_PRE_JS(tulip-node.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_pre.js)
EM_LINK_POST_JS(tulip-node.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_post.js)
EM_LINK_JS_LIBRARY(tulip-node.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_library.js)

ADD_CUSTOM_COMMAND(TARGET tulip-node.js POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -DTULIP_VIZ_FEATURES=OFF -DJSFILE=tulip-node.js -DJSUTILSFILE=${CMAKE_SOURCE_DIR}/src/emscripten/utils.js -P ${CMAKE_SOURCE_DIR}/src/emscripten/wrap_compiled_code.cmake
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

ADD_EXECUTABLE(tulip.js ${TULIPJS_SRCS})

TARGET_LINK_LIBRARIES(tulip.js ${TULIP_CORE_LIBRARY} ${TULIP_OGDF_LIBRARY} ${TULIP_PLUGINS_LIBRARIES})

SET_TARGET_PROPERTIES(tulip.js PROPERTIES LINK_FLAGS "-s STB_IMAGE=1 --bind -s USE_ZLIB=1 -s USE_FREETYPE=1 -s LZ4=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=\\\"tulipjs\\\" --preload-file resources/")

EM_LINK_PRE_JS(tulip.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_pre.js)
EM_LINK_POST_JS(tulip.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_post.js)
EM_LINK_JS_LIBRARY(tulip.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_library.js)

ADD_CUSTOM_COMMAND(TARGET tulip.js POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -DTULIP_VIZ_FEATURES=ON -DJSFILE=tulip.js -DJSUTILSFILE=${CMAKE_SOURCE_DIR}/src/emscripten/utils.js -P ${CMAKE_SOURCE_DIR}/src/emscripten/wrap_compiled_code.cmake
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

ADD_EXECUTABLE(tulip-lite.js ${TULIPJS_SRCS})

TARGET_LINK_LIBRARIES(tulip-lite.js ${TULIP_CORE_LIBRARY})

SET_TARGET_PROPERTIES(tulip-lite.js PROPERTIES LINK_FLAGS "-s STB_IMAGE=1 --bind -s USE_ZLIB=1 -s USE_FREETYPE=1 -s LZ4=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=\\\"tulipjs\\\" --preload-file resources/")

EM_LINK_PRE_JS(tulip-lite.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_pre.js)
EM_LINK_POST_JS(tulip-lite.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_post.js)
EM_LINK_JS_LIBRARY(tulip-lite.js ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_library.js)

ADD_CUSTOM_COMMAND(TARGET tulip-lite.js POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -DTULIP_VIZ_FEATURES=ON -DJSFILE=tulip-lite.js -DJSUTILSFILE=${CMAKE_SOURCE_DIR}/src/emscripten/utils.js -P ${CMAKE_SOURCE_DIR}/src/emscripten/wrap_compiled_code.cmake
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_webix.html DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_web.html DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_web_multi.html DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_js_test.html DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_graph_traversals.html DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/FileSaver.js DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/spinner.css DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/images DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/images/favicon.ico DESTINATION ${CMAKE_INSTALL_PREFIX})

IF(BUILD_TYPE MATCHES "release")
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip.js DESTINATION ${CMAKE_SOURCE_DIR}/dist)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip.js.mem DESTINATION ${CMAKE_SOURCE_DIR}/dist)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip.data DESTINATION ${CMAKE_SOURCE_DIR}/dist)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip-lite.js DESTINATION ${CMAKE_SOURCE_DIR}/dist)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip-lite.js.mem DESTINATION ${CMAKE_SOURCE_DIR}/dist)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip-lite.data DESTINATION ${CMAKE_SOURCE_DIR}/dist)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip-node.js DESTINATION ${CMAKE_SOURCE_DIR}/dist)

  SET(DEMO_WEBSITE_LOCAL_REPO ${CMAKE_SOURCE_DIR}/../anlambert.github.io)

  IF(EXISTS ${DEMO_WEBSITE_LOCAL_REPO})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip.js DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip.js.mem DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/tulip.data DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)

    INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_webix.html DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_web.html DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_web_multi.html DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_js_test.html DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/tulip_graph_traversals.html DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/FileSaver.js DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/emscripten/spinner.css DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/images DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/images/favicon.ico DESTINATION ${DEMO_WEBSITE_LOCAL_REPO}/tulipjs/)
  ENDIF(EXISTS ${DEMO_WEBSITE_LOCAL_REPO})

ENDIF(BUILD_TYPE MATCHES "release")

ENDIF(NOT EMSCRIPTEN)
