#ifndef METEOR_FRAG_UTILS
#define METEOR_FRAG_UTILS

out vec4 FragColor;  // Uniform color

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