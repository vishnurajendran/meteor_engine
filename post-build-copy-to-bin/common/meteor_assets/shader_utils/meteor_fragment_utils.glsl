#ifndef METEOR_FRAG_UTILS
#define METEOR_FRAG_UTILS

out vec4 FragColor;  // Uniform color

// MVP matrices — declared here so fragment shaders can access view/model/projection.
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