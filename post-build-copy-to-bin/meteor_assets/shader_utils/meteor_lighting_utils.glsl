#include </meteor_fragment_utils.glsl>

#ifndef METEOR_LIGHTING_UTILS
#define METEOR_LIGHTING_UTILS

const int MAX_DYN_LIGHTS = 8;

layout (std140, binding = 1) uniform AmbientLight {
    vec3 color;
    float intensity;
    int enabled;
    float padding[3];
} uAmbientLight;

layout (std140, binding=2) uniform DirectionalLight{
    vec3 forward;
    float intensity;
    vec3 color;
    int enabled;
}uDirectionalLight;

struct DynamicLightData {
    // common to spot and point lights
    vec3 position;
    float range;
    vec3 color;
    float intensity;
    //specific to spot light
    vec3 direction;
    float angle;
    int type; //0-> none, 1-> point, 2 -> spot
};

layout (std430, binding=3) buffer DynamicLightsContainer {
    DynamicLightData lights[MAX_DYN_LIGHTS];
    int dynLightCount;
}uDynamicLights;

vec3 getDirLightContrib(){
    vec3 vertexNormal = normalize(getVertexNormal());
    vec3 lightForward = normalize(uDirectionalLight.forward);
    float lightContrib = max(dot(lightForward, vertexNormal), 0.0);
    return lightContrib * uDirectionalLight.color * uDirectionalLight.intensity * float(uDirectionalLight.enabled);
}

vec3 getAmbLightContrib(){
     return uAmbientLight.intensity * uAmbientLight.color * float(uAmbientLight.enabled);
}

vec3 getDynamicLightContrib(){
    // Vertex info
    vec3 vWorldPos = getVertexPositionWorld();
    vec3 vWorldNormal = getVertexNormalWorld();

    // Dynamic Point and Spot Lights
    vec3 dynamicLighting = vec3(0.0);

    for (int i = 0; i < uDynamicLights.dynLightCount; i++) {
        DynamicLightData light = uDynamicLights.lights[i];
        //ignore this is a invalid light type
        if (light.type == 0) continue;

        vec3 lightVec = light.position - vWorldPos;
        float distSq = dot(lightVec, lightVec);
        float dist = sqrt(distSq);

        //out of range..
        if (dist > light.range) continue;


        vec3 lightDir = normalize(lightVec);
        float attenuation = pow(1.0 - (dist / light.range), 1.0);
        attenuation = attenuation * attenuation; // quadratic falloff

        float ndotl = max(dot(vWorldNormal, lightDir), 0.0);

        // Spot light check
        if (light.type == 2) {
            float spotCos = dot(normalize(light.direction), lightDir);
            if (spotCos < cos(light.angle)) continue; // outside cone
            // Optional: add smooth falloff inside cone
            attenuation *= smoothstep(cos(light.angle), 1.0, spotCos);
        }

        vec3 lightContrib = ndotl * attenuation * light.color * light.intensity;
        dynamicLighting += lightContrib;
    }

    return dynamicLighting;
}

vec3 applyLighting(vec3 inColor) {

    // Ambient Light
    vec3 ambientLight = getAmbLightContrib();
    // Directional Light
    vec3 directionalLight = getDirLightContrib();
    //Dynamic Lights
    vec3 dynamicLighting = getDynamicLightContrib();

    vec3 totalLight = ambientLight + directionalLight + dynamicLighting;
    return totalLight * inColor;
}

#endif //Shader guard