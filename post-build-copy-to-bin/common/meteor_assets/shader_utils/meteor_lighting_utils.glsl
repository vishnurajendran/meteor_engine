#include </meteor_fragment_utils.glsl>

#ifndef METEOR_LIGHTING_UTILS
#define METEOR_LIGHTING_UTILS

const int MAX_DYN_LIGHTS    = 32;   // must match dynamic_light_shader_container.h
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

// ---- Shadow compute - usePCF is per-instance from the SSBO -----------------

// surfaceNormal (world space) drives the slope-scaled bias. Fullscreen passes
// pass the normal from BUFFER_NORMAL; mesh passes use the overload below.
float computeSpotShadowN(int idx, vec3 worldPos, vec3 surfaceNormal, int usePCF)
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
    // 90° - clamped to prevent full shadow detachment on near-perpendicular faces.
    vec3  toLightDir = normalize(getSpotLightPos(idx) - worldPos);
    float cosTheta   = clamp(dot(surfaceNormal, toLightDir), 0.001, 1.0);
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

float computeSpotShadow(int idx, vec3 worldPos, int usePCF)
{
    return computeSpotShadowN(idx, worldPos, getVertexNormalWorld(), usePCF);
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

// ============================================================================
// Surface lighting - shared by the deferred lighting pass (lighting_pass.mesl)
// and forward-lit transparent shaders (transparent.mesl), so a surface with
// the same material values lights the same way in both paths.
// ============================================================================

// ---- Directional shadow: cascades (bound by MShadowInputs::bind in C++) -----
// The view up to shadowDistance is split into cascadeCount depth ranges, each
// with its own shadow map (one layer of shadowCascades). Near the camera a
// cascade covers a few metres, so its texels are small; far ones cover more.
const int MAX_CASCADES = 4;   // must match SShadowBuffer::MAX_CASCADES

uniform sampler2DArrayShadow shadowCascades;          // compare mode: returns lit fraction, bilinear PCF
uniform mat4  cascadeMatrices[MAX_CASCADES];          // world -> light clip
uniform float cascadeSplits[MAX_CASCADES];            // far edge of each cascade (view distance)
uniform float cascadeTexelWorld[MAX_CASCADES];        // world size of one texel
uniform int   cascadeCount;                           // 0 = no directional shadow
uniform mat4  cascadeCameraView;                      // camera view the splits are measured in
uniform float shadowDistance;
uniform int   showCascades;                           // debug tint
uniform int   hasShadow;
uniform int   hasDirShadow;

// World-space radius of the soft-shadow blur. Converted to texels per
// cascade so soft shadows look the same width near and far.
const float SOFT_SHADOW_WORLD_RADIUS = 0.06;

// 0 = lit, 1 = fully shadowed, for one cascade. -1 if the point falls
// outside this cascade's map (caller picks another cascade or treats as lit).
float sampleShadowCascade(int c, vec3 worldPos, vec3 N, vec3 L)
{
    float texel = cascadeTexelWorld[c];

    // Normal offset: push the lookup point off the surface by about a texel
    // (more at grazing angles). Removes acne without the peter-panning a
    // large depth bias causes, and scales with each cascade's texel size.
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    vec3  p     = worldPos + N * texel * (1.0 + 2.0 * (1.0 - NdotL));

    vec4 ls = cascadeMatrices[c] * vec4(p, 1.0);
    vec3 pc = ls.xyz / ls.w * 0.5 + 0.5;
    if (pc.x < 0.0 || pc.x > 1.0 || pc.y < 0.0 || pc.y > 1.0) return -1.0;
    if (pc.z > 1.0) return 0.0;                        // beyond the cascade's far plane

    float ref = pc.z - 0.0002;
    float layer = float(c);

    if (smoothShadows == 1)
    {
        vec2  texelUV  = 1.0 / vec2(textureSize(shadowCascades, 0).xy);
        float spreadPx = clamp(SOFT_SHADOW_WORLD_RADIUS / texel, 1.0, 6.0);
        vec2  spread   = texelUV * spreadPx;

        float lit = 0.0;
        for (int i = 0; i < 16; ++i)
            lit += texture(shadowCascades, vec4(pc.xy + poissonDisk[i] * spread, layer, ref));
        return 1.0 - lit / 16.0;
    }

    // Hardware 2x2 bilinear PCF.
    return 1.0 - texture(shadowCascades, vec4(pc.xy, layer, ref));
}

int selectShadowCascade(float viewDepth)
{
    for (int i = 0; i < cascadeCount; ++i)
        if (viewDepth < cascadeSplits[i]) return i;
    return cascadeCount - 1;
}

// 0 = lit, 1 = fully shadowed. N is the world-space surface normal.
float computeDirectionalShadow(vec3 worldPos, vec3 N)
{
    if (hasDirShadow == 0 || cascadeCount == 0) return 0.0;

    float viewDepth = -(cascadeCameraView * vec4(worldPos, 1.0)).z;
    if (viewDepth >= shadowDistance) return 0.0;

    vec3 L = normalize(uDirectionalLight.forward);
    int  c = selectShadowCascade(viewDepth);
    float shadow = max(sampleShadowCascade(c, worldPos, N, L), 0.0);

    // Blend into the next cascade over the last 10% of this one, so the
    // resolution change isn't a visible line.
    if (c < cascadeCount - 1)
    {
        float start = (c == 0) ? 0.0 : cascadeSplits[c - 1];
        float end   = cascadeSplits[c];
        float band  = (end - start) * 0.1;
        if (viewDepth > end - band)
        {
            float t    = (viewDepth - (end - band)) / band;
            float next = sampleShadowCascade(c + 1, worldPos, N, L);
            if (next >= 0.0) shadow = mix(shadow, next, t);
        }
    }

    // Fade out over the last 10% of the shadow distance.
    float fade = 1.0 - smoothstep(shadowDistance * 0.9, shadowDistance, viewDepth);
    return shadow * fade;
}

// Debug colour per cascade (Show Cascades on the directional light).
vec3 cascadeDebugTint(vec3 worldPos)
{
    if (cascadeCount == 0) return vec3(1.0);
    float viewDepth = -(cascadeCameraView * vec4(worldPos, 1.0)).z;
    if (viewDepth >= shadowDistance) return vec3(1.0);
    int c = selectShadowCascade(viewDepth);
    if (c == 0) return vec3(1.0, 0.45, 0.45);
    if (c == 1) return vec3(0.45, 1.0, 0.45);
    if (c == 2) return vec3(0.45, 0.6, 1.0);
    return vec3(1.0, 1.0, 0.45);
}

// Light arriving at a surface, before albedo:
//   ambient + directional (with shadow) + every point/spot light in
//   uDynamicLights (with shadows), diffuse + Blinn-Phong specular.
// Multiply the result by the surface albedo to get the lit colour.
//
//   worldPos   - surface position, world space
//   N          - unit surface normal, world space (normal map applied)
//   cameraPos  - world-space camera position
//   metallic   - 0 dielectric .. 1 metal (shifts diffuse into specular)
//   smoothness - 0 rough .. 1 mirror-like (highlight tightness)
vec3 meteorSurfaceLighting(vec3 worldPos, vec3 N, vec3 cameraPos,
                           float metallic, float smoothness)
{
    vec3 V = normalize(cameraPos - worldPos);

    // smoothness 0 -> power 2 (very rough), 1 -> power 2048 (near-mirror)
    float specPower = pow(2.0, smoothness * 10.0 + 1.0);

    // Dielectric F0 ~= 0.04, metallic F0 ~= 1.0. Multiplying by albedo
    // afterwards tints metallic specular by the base colour, which is right;
    // for dielectrics it is a known approximation (should be white).
    float specWeight = mix(0.04, 1.0, metallic);

    // -- Ambient --
    vec3 ambient = uAmbientLight.intensity * uAmbientLight.color
                   * float(uAmbientLight.enabled);

    // -- Directional --
    vec3 dirDiffuse  = vec3(0.0);
    vec3 dirSpecular = vec3(0.0);
    if (uDirectionalLight.enabled == 1)
    {
        vec3  L     = normalize(uDirectionalLight.forward);
        float NdotL = max(dot(L, N), 0.0);
        dirDiffuse  = NdotL * uDirectionalLight.color * uDirectionalLight.intensity;

        vec3  H     = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        // Specular only where the surface faces the light
        dirSpecular = pow(NdotH, specPower) * step(0.001, NdotL)
                      * uDirectionalLight.color * uDirectionalLight.intensity;

        float shadow = computeDirectionalShadow(worldPos, N);
        dirDiffuse  *= (1.0 - shadow);
        dirSpecular *= (1.0 - shadow);
    }

    // -- Point + spot --
    vec3 dynDiffuse  = vec3(0.0);
    vec3 dynSpecular = vec3(0.0);
    for (int i = 0; i < uDynamicLights.dynLightCount; i++)
    {
        DynamicLightData light = uDynamicLights.lights[i];
        if (light.type == 0) continue;

        vec3  lightVec = light.position - worldPos;
        float dist     = length(lightVec);
        if (dist > light.range) continue;

        vec3  lightDir = normalize(lightVec);
        float atten    = pow(1.0 - (dist / light.range), 2.0);
        float ndotl    = max(dot(N, lightDir), 0.0);

        if (light.type == 2) // spot
        {
            float spotCos = dot(normalize(light.direction), lightDir);
            if (spotCos < cos(light.angle)) continue;
            atten *= smoothstep(cos(light.angle), 1.0, spotCos);
        }

        float shadowVal = 0.0;
        if (light.shadowIndex >= 0)
        {
            if (light.type == 2)
                shadowVal = computeSpotShadowN(light.shadowIndex, worldPos, N, light.smoothShadow);
            else if (light.type == 1)
                shadowVal = computePointShadow(light.shadowIndex, worldPos, light.position, light.smoothShadow);
        }

        float lightScale = atten * (1.0 - shadowVal);
        dynDiffuse += ndotl * lightScale * light.color * light.intensity;

        vec3  H     = normalize(lightDir + V);
        float NdotH = max(dot(N, H), 0.0);
        dynSpecular += pow(NdotH, specPower) * step(0.001, ndotl)
                       * lightScale * light.color * light.intensity;
    }

    // Metallic 0: full diffuse, specular x0.04. Metallic 1: no diffuse, specular x1.
    vec3 totalDiffuse  = (ambient + dirDiffuse + dynDiffuse) * (1.0 - metallic);
    vec3 totalSpecular = (dirSpecular + dynSpecular) * specWeight;
    vec3 total         = totalDiffuse + totalSpecular;

    if (showCascades == 1)
        total *= cascadeDebugTint(worldPos);

    return total;
}

#endif
