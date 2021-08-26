#include "material.h"
#include "shadersource.h"
#include <exception>
#include <GL/gl3w.h>
#include "world.h"

namespace diorama {

const Texture Texture::NO_TEXTURE = Texture();

void ShaderManager::init()
{
    basicVert = compileShader(GL_VERTEX_SHADER, "Vertex",
        {VERSION_DIRECTIVE, vertShaderSrc});
    GLShader coloredFrag = compileShader(GL_FRAGMENT_SHADER, "Fragment",
        {VERSION_DIRECTIVE, fragShaderSrc});
    GLShader texturedFrag = compileShader(GL_FRAGMENT_SHADER, "Fragment",
        {VERSION_DIRECTIVE,
        "#define BASE_TEXTURE\n",
        fragShaderSrc});
    GLShader shiftedTextureFrag = compileShader(GL_FRAGMENT_SHADER, "Fragment",
        {VERSION_DIRECTIVE,
        "#define BASE_TEXTURE\n#define COLORIZE_SHIFT\n",
        fragShaderSrc});
    GLShader tintedTextureFrag = compileShader(GL_FRAGMENT_SHADER, "Fragment",
        {VERSION_DIRECTIVE,
        "#define BASE_TEXTURE\n#define COLORIZE_TINT\n",
        fragShaderSrc});
    GLShader debugFrag = compileShader(GL_FRAGMENT_SHADER, "Fragment",
        {VERSION_DIRECTIVE, debugFragShaderSrc});

    coloredProg.glProgram = linkProgram("Program", {basicVert, coloredFrag});
    setProgramBindings(coloredProg);

    texturedProg.glProgram = linkProgram("Program", {basicVert, texturedFrag});
    setProgramBindings(texturedProg);

    shiftedTextureProg.glProgram = linkProgram("Program",
        {basicVert, shiftedTextureFrag});
    setProgramBindings(shiftedTextureProg);

    tintedTextureProg.glProgram = linkProgram("Program",
        {basicVert, tintedTextureFrag});
    setProgramBindings(tintedTextureProg);

    debugProg.glProgram = linkProgram("Program", {basicVert, debugFrag});
    setProgramBindings(debugProg);

    glDeleteShader(coloredFrag);
    glDeleteShader(texturedFrag);
    glDeleteShader(shiftedTextureFrag);
    glDeleteShader(tintedTextureFrag);
    glDeleteShader(debugFrag);
}

GLShader ShaderManager::compileShader(GLConst type, string name,
        std::initializer_list<string> sources)
{
    GLShader shader = glCreateShader(type);
    vector<const char *> sourcePtrs;
    sourcePtrs.reserve(sources.size());
    for (auto &source : sources)
        sourcePtrs.push_back(source.c_str());

    glShaderSource(shader, sourcePtrs.size(), &sourcePtrs[0], nullptr);

    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        unique_ptr<char[]> log(new char[logLen]);
        glGetShaderInfoLog(shader, logLen, NULL, log.get());
        printf("%s compile error: %s", name.c_str(), log.get());
        throw std::exception("Shader compile error");
    }
    return shader;
}

GLProgram ShaderManager::linkProgram(string name,
        std::initializer_list<GLShader> shaders)
{
    GLProgram program = glCreateProgram();
    for (auto &shader : shaders)
        glAttachShader(program, shader);

    glLinkProgram(program);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint logLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        unique_ptr<char[]> log(new char[logLen]);
        glGetProgramInfoLog(program, logLen, NULL, log.get());
        printf("%s link error: %s", name.c_str(), log.get());
        throw std::exception("Program link error");
    }
    return program;
}

void ShaderManager::setProgramBindings(ShaderProgram &program)
{
    program.baseColorLoc = glGetUniformLocation(
        program.glProgram, "BaseColor");
    program.textureScaleLoc = glGetUniformLocation(
        program.glProgram, "TextureScale");

    glUseProgram(program.glProgram);
    GLuint transformIdx = glGetUniformBlockIndex(
        program.glProgram, "TransformBlock");
    glUniformBlockBinding(program.glProgram,
        transformIdx, ShaderProgram::BIND_TRANSFORM);

    GLUniformLocation baseTextureLoc = glGetUniformLocation(
        program.glProgram, "BaseTexture");
    glUniform1i(baseTextureLoc, Material::TEXTURE_BASE);
}

}  // namespace
