#include "ShaderManager.h"
#include "GlShaderProgram.h"

using namespace std;

map<string, ShaderManager *> ShaderManager::_instances;
string ShaderManager::_currentCanvasId("");


ShaderManager *ShaderManager::getInstance(const std::string &canvasId) {
    if (_instances.find(canvasId) == _instances.end()) {
        _instances[canvasId] = new ShaderManager();
    }
    return _instances[canvasId];
}

ShaderManager *ShaderManager::getInstance() {
    return getInstance(_currentCanvasId);
}

ShaderManager::ShaderManager() {
    _defaultRenderingShader = new GlShaderProgram();
    _defaultRenderingShader->addShaderFromSourceCode(GlShader::Vertex, defaultVertexShaderSrc);
    _defaultRenderingShader->addShaderFromSourceCode(GlShader::Fragment, defaultFragmentShaderSrc);
    _defaultRenderingShader->link();
    if (!_defaultRenderingShader->isLinked()) {
        _defaultRenderingShader->printInfoLog();
    }

    _blinnPhongRenderingShader = new GlShaderProgram();
    _blinnPhongRenderingShader->addShaderFromSourceCode(GlShader::Vertex, blinnPhongVertexShaderSrc);
    _blinnPhongRenderingShader->addShaderFromSourceCode(GlShader::Fragment, blinnPhongFragmentShaderSrc);
    _blinnPhongRenderingShader->link();
    if (!_blinnPhongRenderingShader->isLinked()) {
        _blinnPhongRenderingShader->printInfoLog();
    }

}
