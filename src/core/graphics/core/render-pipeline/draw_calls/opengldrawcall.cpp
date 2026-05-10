//
// Created by Vishnu Rajendran on 2024-09-25.
//
#include <GL/glew.h>
#include "SFML/OpenGL.hpp"
#include "opengldrawcall.h"

void MOpenGlDrawCall::draw() {
    glBegin(GL_TRIANGLES);
    glVertex2f(0.0f,0.5);
    glVertex2f(-0.5f,-0.5f);
    glVertex2f(0.5f,-0.5f);
    glEnd();
}

void MOpenGlDrawCall::setTargetResolution(SVector2 resolution) {
    this->resolution = resolution;
}

void MOpenGlDrawCall::printOpenGLDrawErrors() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        switch (err) {
            case GL_INVALID_ENUM:
                MERROR("OpenGL Error: GL_INVALID_ENUM");
            break;
            case GL_INVALID_VALUE:
                MERROR("OpenGL Error: GL_INVALID_VALUE");
            break;
            case GL_INVALID_OPERATION:
               MERROR("OpenGL Error: GL_INVALID_OPERATION");
            break;
            case GL_STACK_OVERFLOW:
                MERROR("OpenGL Error: GL_STACK_OVERFLOW");
            break;
            case GL_STACK_UNDERFLOW:
                MERROR("OpenGL Error: GL_STACK_UNDERFLOW");
            break;
            case GL_OUT_OF_MEMORY:
               MERROR("OpenGL Error: GL_OUT_OF_MEMORY");
            break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                MERROR("OpenGL Error: GL_INVALID_FRAMEBUFFER_OPERATION");
            break;
            default:
                MERROR("OpenGL Error: Unknown Error");
            break;
        }
    }
}
