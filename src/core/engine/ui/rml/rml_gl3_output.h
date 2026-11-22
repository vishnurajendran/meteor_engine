//
// rml_gl3_output.h
//
// The framebuffer RmlUi's GL3 renderer draws its finished frame into.
//
// The stock renderer (Backends/RmlUi_Renderer_GL3.cpp) composites onto the
// default framebuffer (0). The engine draws UI into whatever the pipeline
// targets - the window in a player, the Scene view's render texture in the
// editor - so CMake compiles a copy of that file with every
// glBindFramebuffer(..., 0) replaced by this function's result, and with
// GLEW as its GL loader. The render stage sets the target before each frame.
//
// Included by that generated source; keep it C++ with no engine includes.
//

#ifndef RML_GL3_OUTPUT_H
#define RML_GL3_OUTPUT_H

unsigned int RmlGL3_GetOutputFramebuffer();
void         RmlGL3_SetOutputFramebuffer(unsigned int framebuffer);

#endif // RML_GL3_OUTPUT_H
