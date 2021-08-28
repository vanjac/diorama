#include "material.h"
#include "shadersource.h"
#include <cstdio>
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

void ShaderProgram::link(string name, initializer_list<GLShader> shaders) {
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

    modelMatrixLoc = glGetUniformLocation(glProgram, "ModelMatrix");
    normalMatrixLoc = glGetUniformLocation(glProgram, "NormalMatrix");
    baseColorLoc = glGetUniformLocation(glProgram, "BaseColor");
    textureScaleLoc = glGetUniformLocation(glProgram, "TextureScale");

    glUseProgram(glProgram);
    GLuint transformIdx = glGetUniformBlockIndex(glProgram, "CameraBlock");
    glUniformBlockBinding(glProgram, transformIdx, BIND_TRANSFORM);

    GLUniformLocation baseTextureLoc = glGetUniformLocation(
        glProgram, "BaseTexture");
    glUniform1i(baseTextureLoc, Material::TEXTURE_BASE);

    glUseProgram(0);
}

void ShaderManager::linkPrograms()
{
    basicVert = compileShader(GLShaderType::VertexShader,
        "Basic vertex",
        {VERSION_DIRECTIVE, vertShaderSrc});
    GLShader coloredFrag = compileShader(GLShaderType::FragmentShader,
        "Solid color",
        {VERSION_DIRECTIVE, fragShaderSrc});
    GLShader texturedFrag = compileShader(GLShaderType::FragmentShader,
        "Textured",
        {VERSION_DIRECTIVE,
        "#define BASE_TEXTURE\n",
        fragShaderSrc});
    GLShader shiftedTextureFrag = compileShader(GLShaderType::FragmentShader,
        "Color-shifted texture",
        {VERSION_DIRECTIVE,
        "#define BASE_TEXTURE\n#define COLORIZE_SHIFT\n",
        fragShaderSrc});
    GLShader tintedTextureFrag = compileShader(GLShaderType::FragmentShader,
        "Color-tinted texture",
        {VERSION_DIRECTIVE,
        "#define BASE_TEXTURE\n#define COLORIZE_TINT\n",
        fragShaderSrc});
    GLShader debugFrag = compileShader(GLShaderType::FragmentShader,
        "Debug",
        {VERSION_DIRECTIVE, debugFragShaderSrc});

    coloredProg.link("Solid color", {basicVert, coloredFrag});
    texturedProg.link("Textured", {basicVert, texturedFrag});
    shiftedTextureProg.link("Color-shifted texture",
        {basicVert, shiftedTextureFrag});
    tintedTextureProg.link("Color-tinted texture",
        {basicVert, tintedTextureFrag});
    debugProg.link("Debug", {basicVert, debugFrag});

    glDeleteShader(coloredFrag);
    glDeleteShader(texturedFrag);
    glDeleteShader(shiftedTextureFrag);
    glDeleteShader(tintedTextureFrag);
    glDeleteShader(debugFrag);
}

GLShader ShaderManager::compileShader(GLShaderType type, string name,
                                      initializer_list<string> sources)
{
    GLShader shader = glCreateShader((GLenum)type);
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

void Texture::setImage(int width, int height, GLTextureFormat format,
                       GLDataType type, const void *data)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 width, height, 0,
                 (GLenum)format, (GLenum)type, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);  // trilinear
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace
