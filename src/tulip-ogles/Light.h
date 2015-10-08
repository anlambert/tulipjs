#ifndef LIGHT_H
#define LIGHT_H

#include <tulip/Vector.h>
#include <tulip/Color.h>

class Camera;

class Light {

public:

    Light();
    Light(Camera *camera);

    void setCamera(Camera *camera) {
      _camera = camera;
    }

    void setDirectionnalLight(const bool directionnalLight);
    bool directionnalLight() const;

    tlp::Vec4f getPosition() const;

    void setModelAmbientColor(const tlp::Color &modelAmbient);
    tlp::Color getModelAmbientColor() const;

    void setAmbientColor(const tlp::Color &ambient);
    tlp::Color getAmbientColor() const;

    void setDiffuseColor(const tlp::Color &diffuse);
    tlp::Color getDiffuseColor() const;

    void setSpecularColor(const tlp::Color &specular);
    tlp::Color getSpecularColor() const;

    void setConstantAttenuation(const float constantAttenuation);
    float getConstantAttenuation() const;

    void setLinearAttenuation(const float linearAttenuation);
    float getLinearAttenuation() const;

    void setQuadraticAttenuation(const float quadraticAttenuation);
    float getQuadraticAttenuation() const;


private:

    Camera *_camera;
    bool _directionnalLight;
    tlp::Color _modelAmbientColor;
    tlp::Color _ambientColor;
    tlp::Color _diffuseColor;
    tlp::Color _specularColor;
    float _constantAttenuation;
    float _linearAttenuation;
    float _quadraticAttenuation;

};

#endif // LIGHT_H
