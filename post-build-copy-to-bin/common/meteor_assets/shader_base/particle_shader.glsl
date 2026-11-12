#ifndef METEOR_PARTICLE_SHADER
#define METEOR_PARTICLE_SHADER

// Base for particle shaders (use with base="meteor_assets/shader_base/particle_shader.glsl").
//
// Drawn instanced by MTransparentStage — one instance per particle.
//   Attributes 0-2: the geometry (a unit quad, or a mesh) — same as every mesh.
//   Attributes 3-7: one particle — from the renderer's instance buffer
//                   (SParticleInstance in particle_renderer.h).
//
// `model` is the emitter's world matrix for Local simulation space and
// identity for Global, so particle positions are always multiplied by it.
//
// _alignment (set by the renderer from its Alignment field):
//   0 billboard          — XY plane faces the camera (sprites). Only a roll
//                          around the view axis survives from the rotation.
//   1 velocity billboard — sprite's +Y along the movement, turned around it
//                          to face the camera as much as it can.
//   2 rotation only      — the geometry is turned by the particle's
//                          quaternion in simulation space (meshes; sprites on Z Axis).
//   3 mesh along velocity — mesh +Z along the movement, then the rotation.
//
// Fragment passes read the particle's colour with getParticleColor() —
// already converted to linear, alpha untouched.

#ifdef COMPILE_VERTEX

#include </meteor_vertex_utils.glsl>
#include </meteor_projection_utils.glsl>

layout(location = 3) in vec3 iParticlePosition;   // simulation space
layout(location = 4) in vec4 iParticleRotation;   // quaternion, x y z w
layout(location = 5) in vec3 iParticleSize;
layout(location = 6) in vec4 iParticleColor;      // sRGB colour + alpha
layout(location = 7) in vec3 iParticleVelocity;   // simulation space

uniform int  _alignment;
uniform vec3 _cameraPos;   // world space — set by MTransparentStage

out vec4 v2fParticleColor;

vec3 meteorRotateByQuat(vec3 v, vec4 q)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

// Exact sRGB -> linear (matches MColorSpace::srgbToLinear).
vec3 meteorSrgbToLinear(vec3 c)
{
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

#ifndef NO_VERTEX_PASS_DEFINED
void meteorVertexPass(inout vec3 vertexPosition, inout vec3 vertexNormal, inout vec2 texCoords);
#else
void meteorVertexPass(inout vec3 vertexPosition, inout vec3 vertexNormal, inout vec2 texCoords) {}
#endif //NO_VERTEX_PASS_DEFINED

void main()
{
    vec3 position  = getVertexPosition();
    vec3 normal    = getNormal();
    vec2 texCoords = getTexCoords();

    // Custom per-vertex code, in the geometry's own space (before size and rotation).
    meteorVertexPass(position, normal, texCoords);

    mat4 modelMatrix = getModelMatrix();
    vec3 local       = meteorRotateByQuat(position * iParticleSize, iParticleRotation);

    vec3 worldPos;
    vec3 worldNormal;
    if (_alignment == 0 || _alignment == 1)
    {
        // Rows of the view matrix's rotation are the camera's axes in world space.
        mat4 v        = getViewMatrix();
        vec3 camRight = vec3(v[0][0], v[1][0], v[2][0]);
        vec3 camUp    = vec3(v[0][1], v[1][1], v[2][1]);
        vec3 camBack  = vec3(v[0][2], v[1][2], v[2][2]);   // points from the scene towards the camera

        vec3 centre = (modelMatrix * vec4(iParticlePosition, 1.0)).xyz;
        vec3 right  = camRight;
        vec3 up     = camUp;

        if (_alignment == 1)
        {
            // Up = direction of travel (world space). Right = perpendicular to
            // both the travel and the line to the camera, so the quad is as
            // camera-facing as it can be while pointing along its movement.
            vec3 velWorld = mat3(modelMatrix) * iParticleVelocity;
            vec3 toCam    = _cameraPos - centre;
            vec3 side     = cross(velWorld, toCam);
            // Not moving, or moving straight at the camera — keep the plain billboard.
            if (dot(velWorld, velWorld) > 1e-10 && dot(side, side) > 1e-10)
            {
                up    = normalize(velWorld);
                right = normalize(side);
            }
        }

        worldPos    = centre + right * local.x + up * local.y;
        worldNormal = _alignment == 1 ? normalize(cross(right, up)) : camBack;
    }
    else if (_alignment == 3)
    {
        // Basis whose +Z is the direction of travel (simulation space).
        vec3 f = iParticleVelocity;
        mat3 basis = mat3(1.0);
        if (dot(f, f) > 1e-10)
        {
            f = normalize(f);
            vec3 ref = abs(f.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);   // avoid a parallel reference
            vec3 r   = normalize(cross(ref, f));
            vec3 u   = cross(f, r);
            basis    = mat3(r, u, f);   // columns: local X, Y, Z
        }
        worldPos    = (modelMatrix * vec4(iParticlePosition + basis * local, 1.0)).xyz;
        worldNormal = normalize(mat3(modelMatrix) * (basis * meteorRotateByQuat(normal, iParticleRotation)));
    }
    else
    {
        worldPos    = (modelMatrix * vec4(iParticlePosition + local, 1.0)).xyz;
        // mat3(model) is fine for rotation + uniform scale; non-uniform emitter
        // scale would need the inverse-transpose.
        worldNormal = normalize(mat3(modelMatrix) * meteorRotateByQuat(normal, iParticleRotation));
    }

    applyPosition(getProjectionMatrix() * getViewMatrix() * vec4(worldPos, 1.0));
    updateNormals(worldNormal);
    passVertexInfoToFragment();
    v2fTexCoord = texCoords;   // keep any change made by meteorVertexPass
    setWorldPos(worldPos);
    setWorldNormals(worldNormal);

    v2fParticleColor = vec4(meteorSrgbToLinear(iParticleColor.rgb), iParticleColor.a);
}

#endif //COMPILE_VERTEX

#ifdef COMPILE_FRAGMENT

#include </meteor_fragment_utils.glsl>
#include </meteor_lighting_utils.glsl>

in vec4 v2fParticleColor;

vec4 getParticleColor()
{
    return v2fParticleColor;
}

#ifndef NO_FRAGMENT_PASS_DEFINED
vec4 meteorFragmentPass(vec3 vertexPosition, vec3 vertexNormal, vec2 v2fTexCoords);
#else
vec4 meteorFragmentPass(vec3 v2fPosition, vec3 v2fNormal, vec2 v2fTexCoords)
{
    return getParticleColor();
}
#endif

void main()
{
    FragData1  = vec4(0.0);   // transparent pass — no second target bound
    vec4 color = meteorFragmentPass(getVertexPosition(), getVertexNormal(), getTexCoords());
    setFinalOutColor(color);
}

#endif //COMPILE_FRAGMENT

#endif //METEOR_PARTICLE_SHADER
