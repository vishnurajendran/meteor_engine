layout(location = 0) in vec3 aPos;  // Vertex position attribute
layout(location = 1) in vec3 aNormal; // Vertex normal attribute
layout(location = 2) in vec2 aTexCoords; // TexCoords attribute

out vec2 v2fTexCoords;

vec3 getNormal(){
    return aNormal;
}

vec3 getVertexPosition(){
    return aPos;
}

vec2 getTexCoords(){
    return aTexCoords;
}

void passTexCoordsToFragment(vec2 data){
    v2fTexCoords = data;
}