#pragma once

// don't include GL!
#include <cstdint>

namespace diorama {

using GLObject = uint32_t;

using GLBuffer = GLObject;
using GLVertexArray = GLObject;
using GLTexture = GLObject;
using GLShader = GLObject;
using GLProgram = GLObject;

using GLUniformLocation = int32_t;

// replacements for GLenum...

enum GLDataType : uint32_t
{
    GLUnsignedByte = 0x1401,
    GLFloat = 0x1406,
};

enum GLShaderType : uint32_t
{
    GLVertexShader = 0x8B31,
    GLFragmentShader = 0x8B30,
};

enum GLTextureFormat : uint32_t
{
    GLRgba = 0x1908,
};

}  // namespace
