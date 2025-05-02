#ifndef METEOR_STANDARD_SHADER
#define METEOR_STANDARD_SHADER

#ifdef COMPILE_VERTEX

#include </meteor_vertex_utils.glsl>
#include </meteor_projection_utils.glsl>

//This is modifiable part of the code.
void meteorVertexPass(inout vec3 vertexPosition, inout vec3 vertexNormal, inout vec2 texCoords);

void main()
{
    // Cache values.
    vec3 position = getVertexPosition();
    vec3 normal = getNormal();
    vec2 texCoords = getTexCoords();

    // custom code from vertex pass provided by meteor
    meteorVertexPass(position, normal, texCoords);

    // apply projection to position and normals
    vec4 projectedPosition = applyProjection(vec4(position, 1.0));
    vec3 projectedNormals = mat3(transpose(inverse(getModelMatrix()))) * normal;

    // apply positions, normals and send data to fragment.
    applyPosition(projectedPosition);
    updateNormals(projectedNormals);
    passVertexInfoToFragment();
    setWorldPos((getModelMatrix() * vec4(position,1.0)).xyz);

    mat3 normalMatrix = transpose(inverse(mat3(getModelMatrix())));
    setWorldNormals(normalize(normalMatrix * normal));
}

#ifdef NO_VERTEX_PASS_DEFINED
// if no function is defined, put this empty function.
void meteorVertexPass(inout vec3 vertexPosition, inout vec3 vertexNormal, inout vec3 vertexTexCoords) {}
#endif //NO_VERTEX_PASS_DEFINED

#endif //COMPILE_VERTEX

#ifdef COMPILE_FRAGMENT

#include </meteor_fragment_utils.glsl>
#include </meteor_lighting_utils.glsl>

vec4 meteorFragmentPass(vec3 vertexPosition, vec3 vertexNormal, vec2 v2fTexCoords);

void main(){

    //set the final output color from the
    //output of shader.
    vec4 color = meteorFragmentPass(getVertexPosition(), getVertexNormal(), getTexCoords());
    setFinalOutColor(color);
}

#ifdef NO_FRAGMENT_PASS_DEFINED
// set a magenta color to show invalid shader.
vec4 meteorFragmentPass(vec3 v2fPosition, vec3 v2fNormal, vec2 v2fTexCoords) { return vec4(1.0, 0.0, 1.0, 1.0);}
#endif //NO_FRAGMENT_PASS_DEFINED

#endif //COMPILE_FRAGMENT

#endif //METEOR_STANDARD_SHADER