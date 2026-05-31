#include </meteor_fragment_utils.glsl>

#ifndef METEOR_LIGHTING_UTILS
#define METEOR_LIGHTING_UTILS

const int MAX_DYN_LIGHTS    = 8;
const int MAX_SPOT_SHADOWS  = 4;
const int MAX_POINT_SHADOWS = 4;

layout (std140, binding = 1) uniform AmbientLight {
    vec3  color;
    float intensity;
    int   enabled;
    float padding[3];
} uAmbientLight;

layout (std140, binding=2) uniform DirectionalLight {
    vec3  forward;
    float intensity;
    vec3  color;
    int   enabled;
} uDirectionalLight;

struct DynamicLightData {
    vec3  position;
    float range;
    vec3  color;
    float intensity;
    vec3  direction;
    float angle;
    int   type;         // 0=none, 1=point, 2=spot
    int   shadowIndex;  // -1=no shadow
    int   smoothShadow; // 1=Poisson PCF for this instance
    int   _pad1;
};

layout (std430, binding=3) buffer DynamicLightsContainer {
    DynamicLightData lights[MAX_DYN_LIGHTS];
    int dynLightCount;
} uDynamicLights;

// smoothShadows: directional-only global PCF toggle (declared here, used in lighting_pass.mesl too)
uniform int smoothShadows;

const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870), vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845), vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554), vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507), vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367), vec2( 0.14383161, -0.14100790)
);

// ---- Sampler helpers -------------------------------------------------------

uniform sampler2D   spotShadowMap0;
uniform sampler2D   spotShadowMap1;
uniform sampler2D   spotShadowMap2;
uniform sampler2D   spotShadowMap3;
uniform mat4        spotLightSpaceMatrix0;
uniform mat4        spotLightSpaceMatrix1;
uniform mat4        spotLightSpaceMatrix2;
uniform mat4        spotLightSpaceMatrix3;
uniform vec3        spotLightPos0;
uniform vec3        spotLightPos1;
uniform vec3        spotLightPos2;
uniform vec3        spotLightPos3;
uniform float       spotFarPlane0;
uniform float       spotFarPlane1;
uniform float       spotFarPlane2;
uniform float       spotFarPlane3;

uniform samplerCube pointShadowMap0;
uniform samplerCube pointShadowMap1;
uniform samplerCube pointShadowMap2;
uniform samplerCube pointShadowMap3;
uniform float       pointFarPlane0;
uniform float       pointFarPlane1;
uniform float       pointFarPlane2;
uniform float       pointFarPlane3;

float sampleSpotDepth(int idx, vec2 uv)
{
    if (idx == 0) return texture(spotShadowMap0, uv).r;
    if (idx == 1) return texture(spotShadowMap1, uv).r;
    if (idx == 2) return texture(spotShadowMap2, uv).r;
    return texture(spotShadowMap3, uv).r;
}

mat4 getSpotLightSpaceMatrix(int idx)
{
    if (idx == 0) return spotLightSpaceMatrix0;
    if (idx == 1) return spotLightSpaceMatrix1;
    if (idx == 2) return spotLightSpaceMatrix2;
    return spotLightSpaceMatrix3;
}

vec3 getSpotLightPos(int idx)
{
    if (idx == 0) return spotLightPos0;
    if (idx == 1) return spotLightPos1;
    if (idx == 2) return spotLightPos2;
    return spotLightPos3;
}

float getSpotFarPlane(int idx)
{
    if (idx == 0) return spotFarPlane0;
    if (idx == 1) return spotFarPlane1;
    if (idx == 2) return spotFarPlane2;
    return spotFarPlane3;
}

float samplePointDepth(int idx, vec3 dir)
{
    if (idx == 0) return texture(pointShadowMap0, dir).r;
    if (idx == 1) return texture(pointShadowMap1, dir).r;
    if (idx == 2) return texture(pointShadowMap2, dir).r;
    return texture(pointShadowMap3, dir).r;
}

float getPointFarPlane(int idx)
{
    if (idx == 0) return pointFarPlane0;
    if (idx == 1) return pointFarPlane1;
    if (idx == 2) return pointFarPlane2;
    return pointFarPlane3;
}

// ---- Shadow compute — usePCF is per-instance from the SSBO -----------------

float computeSpotShadow(int idx, vec3 worldPos, int usePCF)
{
    if (idx < 0 || idx >= MAX_SPOT_SHADOWS) return 0.0;

    vec4 fragLS     = getSpotLightSpaceMatrix(idx) * vec4(worldPos, 1.0);
    vec3 projCoords = fragLS.xyz / fragLS.w;
    projCoords      = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0)
        return 0.0;

    float far         = getSpotFarPlane(idx);
    float currentDist = length(worldPos - getSpotLightPos(idx));
    // Slope-scaled bias: when the light hits a surface at a grazing angle
    // (dot product near 0) the shadow map texels stretch enormously and a
    // flat bias causes stripes. tan(acos(cosTheta)) grows toward infinity at
    // 90° — clamped to prevent full shadow detachment on near-perpendicular faces.
    vec3  toLightDir = normalize(getSpotLightPos(idx) - worldPos);
    float cosTheta   = clamp(dot(getVertexNormalWorld(), toLightDir), 0.001, 1.0);
    float bias       = clamp(0.05 * tan(acos(cosTheta)), 0.02, 0.8);

    if (usePCF == 1)
    {
        vec2  texelSize = 1.0 / vec2(textureSize(spotShadowMap0, 0));
        float spread    = texelSize.x * 2.0;
        float shadow    = 0.0;
        for (int i = 0; i < 16; ++i)
        {
            float d = sampleSpotDepth(idx, projCoords.xy + poissonDisk[i] * spread);
            shadow += (currentDist - bias > d * far) ? 1.0 : 0.0;
        }
        return shadow / 16.0;
    }
    else
    {
        float d = sampleSpotDepth(idx, projCoords.xy);
        return (currentDist - bias > d * far) ? 1.0 : 0.0;
    }
}

float computePointShadow(int idx, vec3 worldPos, vec3 lightPos, int usePCF)
{
    if (idx < 0 || idx >= MAX_POINT_SHADOWS) return 0.0;

    vec3  fragToLight = worldPos - lightPos;
    float currentDist = length(fragToLight);
    float far         = getPointFarPlane(idx);
    float bias        = 0.08;

    if (usePCF == 1)
    {
        vec3  n    = normalize(fragToLight);
        vec3  tang = abs(n.x) < 0.9 ? vec3(1,0,0) : vec3(0,1,0);
        vec3  u    = normalize(cross(n, tang));
        vec3  v    = cross(n, u);
        float spread = 0.05;
        float shadow = 0.0;
        for (int i = 0; i < 8; ++i)
        {
            float angle2 = float(i) * (3.14159265 * 0.25);
            vec3  offset = (cos(angle2) * u + sin(angle2) * v) * spread;
            float d = samplePointDepth(idx, fragToLight + offset) * far;
            shadow += (currentDist - bias > d) ? 1.0 : 0.0;
        }
        return shadow / 8.0;
    }
    else
    {
        float d = samplePointDepth(idx, fragToLight) * far;
        return (currentDist - bias > d) ? 1.0 : 0.0;
    }
}

// ---- Light contributions ---------------------------------------------------

vec3 getDirLightContrib()
{
    vec3  vertexNormal = normalize(getVertexNormal());
    vec3  lightForward = normalize(uDirectionalLight.forward);
    float contrib      = max(dot(lightForward, vertexNormal), 0.0);
    return contrib * uDirectionalLight.color * uDirectionalLight.intensity
           * float(uDirectionalLight.enabled);
}

vec3 getAmbLightContrib()
{
    return uAmbientLight.intensity * uAmbientLight.color
           * float(uAmbientLight.enabled);
}

vec3 getDynamicLightContrib()
{
    vec3 vWorldPos    = getVertexPositionWorld();
    vec3 vWorldNormal = getVertexNormalWorld();
    vec3 result       = vec3(0.0);

    for (int i = 0; i < uDynamicLights.dynLightCount; i++)
    {
        DynamicLightData light = uDynamicLights.lights[i];
        if (light.type == 0) continue;

        vec3  lightVec = light.position - vWorldPos;
        float dist     = length(lightVec);
        if (dist > light.range) continue;

        vec3  lightDir = normalize(lightVec);
        float atten    = pow(1.0 - (dist / light.range), 2.0);
        float ndotl    = max(dot(vWorldNormal, lightDir), 0.0);

        if (light.type == 2)
        {
            float spotCos = dot(normalize(light.direction), lightDir);
            if (spotCos < cos(light.angle)) continue;
            atten *= smoothstep(cos(light.angle), 1.0, spotCos);
        }

        float shadow = 0.0;
        if (light.shadowIndex >= 0)
        {
            if (light.type == 2)
                shadow = computeSpotShadow(light.shadowIndex, vWorldPos, light.smoothShadow);
            else if (light.type == 1)
                shadow = computePointShadow(light.shadowIndex, vWorldPos, light.position, light.smoothShadow);
        }

        result += ndotl * atten * light.color * light.intensity * (1.0 - shadow);
    }
    return result;
}

vec3 applyLighting(vec3 inColor)
{
    vec3 ambient     = getAmbLightContrib();
    vec3 directional = getDirLightContrib();
    vec3 dynamic     = getDynamicLightContrib();
    return (ambient + directional + dynamic) * inColor;
}

#endif
