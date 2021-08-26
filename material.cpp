#include "material.h"
#include "shadersource.h"
#include <exception>
#include <GL/gl3w.h>
#include "world.h"

namespace diorama {

const Texture Texture::NO_TEXTURE(0);

ShaderProgram::ShaderProgram()
{
    glProgram = glCreateProgram();
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(glProgram);
}

void ShaderProgram::link(string name, std::initializer_list<GLShader> shaders) {
    for (auto &shader : shaders)
        glAttachShader(glProgram, shader);

    glLinkProgram(glProgram);
    GLint linked;
    glGetProgramiv(glProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint logLen;
        glGetProgramiv(glProgram, GL_INFO_LOG_LENGTH, &logLen);
        unique_ptr<char[]> log(new char[logLen]);
        glGetProgramInfoLog(glProgram, logLen, NULL, log.get());
        printf("%s link error: %s", name.c_str(), log.get());
        throw std::exception("Program link error");
    }
    
    baseColorLoc = glGetUniformLocation(glProgram, "BaseColor");
    textureScaleLoc = glGetUniformLocation(glProgram, "TextureScale");

    glUseProgram(glProgram);
    GLuint transformIdx = glGetUniformBlockIndex(glProgram, "TransformBlock");
    glUniformBlockBinding(glProgram, transformIdx, BIND_TRANSFORM);

    GLUniformLocation baseTextureLoc = glGetUniformLocation(
        glProgram, "BaseTexture");
    glUniform1i(baseTextureLoc, Material::TEXTURE_BASE);
}

ShaderManager::ShaderManager()
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

    coloredProg.link("Program", {basicVert, coloredFrag});
    texturedProg.link("Program", {basicVert, texturedFrag});
    shiftedTextureProg.link("Program", {basicVert, shiftedTextureFrag});
    tintedTextureProg.link("Program", {basicVert, tintedTextureFrag});
    debugProg.link("Program", {basicVert, debugFrag});

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

Texture::Texture()
{
    glGenTextures(1, &glTexture);
}

Texture::Texture(GLTexture glTexture)
    : glTexture(glTexture)
{}

Texture::~Texture()
{
    glDeleteTextures(1, &glTexture);
}

}  // namespace
