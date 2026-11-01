
#ifndef METEOR_PROJECTION_UTILS
#define METEOR_PROJECTION_UTILS

uniform mat4 model;  // Model matrix
uniform mat4 view;   // View matrix
uniform mat4 projection;  // Projection matrix

// Every scene shader computes gl_Position through applyProjection(), so
// declaring it invariant makes the depth prepass and later geometry passes
// produce identical depth for the same mesh (needed for GL_LEQUAL against
// the shared prepass depth).
invariant gl_Position;

vec4 applyProjection(vec4 val) {
    return projection * view * model * val;
}

void applyPosition(vec4 position){
    gl_Position = position;
}

mat4 getModelMatrix(){
    return model;
}

mat4 getViewMatrix(){
    return view;
}

mat4 getProjectionMatrix(){
    return projection;
}

#endif //Shader guard