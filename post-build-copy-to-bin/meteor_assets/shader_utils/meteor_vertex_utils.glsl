
#ifndef METEOR_VERT_UTILS
#define METEOR_VERT_UTILS

layout(location = 0) in vec3 aPos;  // Vertex position attribute
layout(location = 1) in vec3 aNormal; // Vertex normal attribute
layout(location = 2) in vec2 aTexCoords; // TexCoords attribute


// to be sent to 
out vec3 v2fVertPosition;
out vec3 v2fVertNormal;
out vec2 v2fTexCoord;
out vec3 v2fVertPositionWorld;
out vec3 v2fVertNormalWorld;

vec3 getNormal(){
    return aNormal;
}

vec3 getVertexPosition(){
    return aPos;
}

vec2 getTexCoords(){
    return aTexCoords;
}

void updateNormals(vec3 newNormals){
    v2fVertNormal = newNormals;
}

void setWorldNormals(vec3 normals){
    v2fVertNormalWorld = normals;
}


void passVertexInfoToFragment(){
    v2fTexCoord = getTexCoords();
    v2fVertPosition = getVertexPosition();
}

void setWorldPos(vec3 worldPos){
    v2fVertPositionWorld = worldPos;
}

#endif //Shader guard