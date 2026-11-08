#ifndef METEOR_FRAG_UTILS
#define METEOR_FRAG_UTILS

// ---- Outputs ------------------------------------------------------------------
// Output 0: the colour meteorFragmentPass returns.
// Output 1: a second target whose meaning depends on the pass:
//   opaque pass  -> BUFFER_EMISSION: write it with meteorSetEmission(). It is
//                   added after lighting, so emission isn't darkened by
//                   shadow and keeps HDR values for bloom.
//   prepass      -> BUFFER_SURFACE (prepass.mesl writes FragData1 directly).
//   other passes -> no second target bound; writes are discarded.
// The standard base clears it to 0 before meteorFragmentPass runs.
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 FragData1;

// Light the surface gives off by itself (linear, may exceed 1). Opaque
// materials only - transparent shaders add emission to their returned colour.
void meteorSetEmission(vec3 emission)
{
    FragData1 = vec4(emission, 1.0);
}

// MVP matrices - declared here so fragment shaders can access view/model/projection.
// These are also declared in meteor_projection_utils.glsl (vertex only), but the
// fragment stage needs them independently. GLSL allows identical uniform
// declarations in both stages without conflict.
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 v2fVertPosition; // coming from vertex
in vec3 v2fVertPositionWorld; // coming from vertex
in vec3 v2fVertNormal; // coming from vertex
in vec3 v2fVertNormalWorld; //coming from vertex

in vec2 v2fTexCoord; // coming from vertex

void setFinalOutColor(vec4 color){
    FragColor = color;
}

vec2 getTexCoords(){
    return v2fTexCoord;
}

// Shading normal for the side of the surface the camera actually sees.
// For double-sided materials the far side of a triangle is visible, and its
// normal must be flipped. Uses the triangle's real geometry (screen-space
// derivatives) instead of gl_FrontFacing, so meshes imported with reversed
// winding still light correctly.
//   N         - unit shading normal, world space
//   worldPos  - surface position, world space
//   cameraPos - world-space camera position
vec3 meteorFaceForwardNormal(vec3 N, vec3 worldPos, vec3 cameraPos)
{
    vec3 Ng = cross(dFdx(worldPos), dFdy(worldPos));
    if (dot(Ng, Ng) < 1e-20) return N;      // degenerate (e.g. sub-pixel)
    if (dot(Ng, N) < 0.0) Ng = -Ng;         // Ng on the side N points to
    if (dot(Ng, cameraPos - worldPos) < 0.0) N = -N;   // looking at that side's back
    return N;
}

// World-space camera position from the view matrix.
vec3 meteorCameraPositionFromView(mat4 viewMatrix)
{
    return -transpose(mat3(viewMatrix)) * viewMatrix[3].xyz;
}

vec3 getVertexNormal(){
    return v2fVertNormal;
}

vec3 getVertexNormalWorld(){
    return v2fVertNormalWorld;
}

vec3 getVertexPosition(){
    return v2fVertPosition;
}

vec3 getVertexPositionWorld(){
    return v2fVertPositionWorld;
}


#endif //Shader Guard