#include </meteor_fragment_utils.glsl>

#ifndef METEOR_LIGHTING_UTILS
#define METEOR_LIGHTING_UTILS

layout (std140, binding=1) uniform AmbientLight{
    vec3 color;
    float intensity;
}uAmbientLight;

layout (std140, binding=2) uniform DirectionalLight{
    vec3 forward;
    vec3 color;
    float intensity;
}uDirectionalLight;

vec3 applyLighting(vec3 inColor){
    
    //ambient lighting
    vec3 ambientLight = uAmbientLight.intensity * uAmbientLight.color;
    
    //directional lighting
    vec3 vertexPos = getVertexPosition();
    vec3 vertexNormal = normalize(getVertexNormal());
    vec3 lightForward = normalize(uDirectionalLight.forward);
    float lightContrib = dot(lightForward, vertexNormal);
    vec3 directionalLight = max(lightContrib, 0.0) * uDirectionalLight.color * uDirectionalLight.intensity;

    //point lights
    return (ambientLight + directionalLight) * inColor;
}

#endif //Shader guard