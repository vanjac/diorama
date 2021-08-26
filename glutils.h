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

// replacement for GLenum
using GLConst = uint32_t;

}  // namespace
