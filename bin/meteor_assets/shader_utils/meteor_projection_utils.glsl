uniform mat4 model;  // Model matrix
uniform mat4 view;   // View matrix
uniform mat4 projection;  // Projection matrix

vec4 applyProjection(vec4 val) {
    return projection * view * model * val;
}

void applyPosition(vec4 position){
    gl_Position = position;
}