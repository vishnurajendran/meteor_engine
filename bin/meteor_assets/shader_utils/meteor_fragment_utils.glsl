out vec4 FragColor;  // Uniform color
in vec2 v2fTexCoords; // coming from vertex

void setFinalOutColor(vec4 color){
    FragColor = color;
}

vec2 getTexCoords(){
    return v2fTexCoords;
}