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

enum class GLDataType : uint32_t
{
    UnsignedByte = 0x1401,
    Float = 0x1406,
};

enum class GLShaderType : uint32_t
{
    VertexShader = 0x8B31,
    FragmentShader = 0x8B30,
};

enum class GLTextureFormat : uint32_t
{
    Rgba = 0x1908,
};

}  // namespace
