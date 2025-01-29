//
// Created by ssj5v on 24-01-2025.
//

#include "shadercompiler.h"
#include "core/utils/logger.h"
#include <filesystem>
#include <fstream>

bool MShaderCompiler::initialised = false;
bool MShaderCompiler::compileShader(const SString& vertexSource, const SString& fragmentSource,
                                    GLuint& shaderProgramHandle)
{
    if (!initialised)
    {
        initialiseEngine();
    }
    const char* search_directories[] = { "/" };

    // Compile vertex shader
    auto vertex = glCreateShader(GL_VERTEX_SHADER);
    auto vertBuff = vertexSource.c_str();
    GLint vertLength = vertexSource.length();

    //include search directories
    glCompileShaderIncludeARB(vertex, _countof(search_directories),search_directories, nullptr);
    glShaderSource(vertex, 1, &vertBuff, &vertLength);
    glCompileShader(vertex);

    if(!getShaderCompilaionStatus(vertex)) {
        MERROR(STR("ShadeCompiler:: Error compiling vertex shader: ") + getShaderInfoLog(vertex));
        glDeleteShader(vertex);
        return false;
    }

    // Compile fragment shader
    auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragBuff = fragmentSource.c_str();
    GLint fragLength = fragmentSource.length();

    //include search directories
    glCompileShaderIncludeARB(fragment, _countof(search_directories),search_directories, nullptr);
    glShaderSource(fragment, 1, &fragBuff, &fragLength);
    glCompileShader(fragment);

    if(!getShaderCompilaionStatus(fragment)) {
        MERROR(STR("ShadeCompiler:: Error compiling fragment shader: ") + getShaderInfoLog(fragment));
        glDeleteShader(fragment);
        glDeleteShader(vertex);
        return false;
    }

    // Link shaders into a program
    shaderProgramHandle = glCreateProgram();
    glAttachShader(shaderProgramHandle, vertex);
    glAttachShader(shaderProgramHandle, fragment);
    glLinkProgram(shaderProgramHandle);

    // Check if linking was successful
    GLint programSuccess;
    glGetProgramiv(shaderProgramHandle, GL_LINK_STATUS, &programSuccess);
    if (!programSuccess) {
        MERROR(STR("ShadeCompiler:: Error linking shader program: ") + getShaderInfoLog(shaderProgramHandle));
        glDeleteProgram(shaderProgramHandle);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    // Clean up shaders (they are no longer needed after linking)
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return true;
}

bool MShaderCompiler::getShaderCompilaionStatus(const GLuint& shaderId) {
    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    return success == GL_TRUE;
}

SString MShaderCompiler::getShaderInfoLog(const GLuint &shaderId) {
    GLint length = 0;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);

    // Dynamically allocate memory for infoLog
    std::vector<char> infoLog(length + 1);
    glGetShaderInfoLog(shaderId, length, &length, infoLog.data());

    return {infoLog.data()};
}

bool MShaderCompiler::initialiseEngine()
{
    if (initialised)
        return true;

    auto directoryPath = "meteor_assets/shader_headers";
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                if (extension == ".glsl") {
                    registerGLNamedString(entry.path().string(), entry.path().filename().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        MERROR(STR("ShadeCompiler:: Filesystem error: ") + e.what());;
    } catch (const std::exception& e) {
        MERROR(STR("ShadeCompiler:: General error: ") + e.what());
    }

    return true;
}

void MShaderCompiler::registerGLNamedString(const SString& filePath, const SString& fileName)
{
    SString fileData;
    const SString fileNamePath = STR("/") + fileName;
    if (FileIO::readFile(filePath, fileData))
    {
        glNamedStringARB(GL_SHADER_INCLUDE_ARB, -1, fileNamePath.c_str(),-1, fileData.c_str());
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            MERROR(STR("ShadeCompiler:: Error registering named string: ") + std::to_string(err));
        }
    }
}
