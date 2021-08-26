#pragma once
#include "common.h"

#include "glutils.h"
#include "resource.h"
#include <initializer_list>
#include <glm/glm.hpp>

namespace diorama {

// matches interface block in shader
// TODO: split off Model and Normal?
struct TransformBlock
{
    glm::mat4 ModelMatrix {1};
    glm::mat4 NormalMatrix {1};  // actually mat3
    glm::mat4 ViewMatrix {1};
    glm::mat4 ProjectionMatrix {1};
};

class ShaderProgram : Resource
{
public:
    enum BufferBindingPoints
    {
        BIND_TRANSFORM
    };

    ShaderProgram();
    ~ShaderProgram();

    void link(string name, std::initializer_list<GLShader> shaders);

    GLProgram glProgram = 0;
    GLUniformLocation baseColorLoc = -1;
    GLUniformLocation textureScaleLoc = -1;
};

class ShaderManager
{
public:
    ShaderManager();

    ShaderProgram coloredProg;
    ShaderProgram texturedProg;
    ShaderProgram shiftedTextureProg;
    ShaderProgram tintedTextureProg;
    ShaderProgram debugProg;

private:
    GLShader compileShader(GLConst type, string name,
        std::initializer_list<string> sources);

    GLShader basicVert = 0;
};

struct Texture : Resource
{
    static const Texture NO_TEXTURE;

    GLTexture glTexture = 0;
};

struct Material : Resource
{
    enum TextureUnits
    {
        TEXTURE_BASE
    };

    const ShaderProgram *shader = nullptr;  // never null
    bool transparent = false;
    const Texture *texture = nullptr;  // never null
    glm::vec4 color {1, 1, 1, 1};
// https://extensions.sketchup.com/developers/sketchup_c_api/sketchup/struct_s_u_texture_ref.html#ac9341c5de53bcc1a89e51de463bd54a0
    glm::vec2 scale {1, 1};
};

}  // namespace
