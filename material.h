#pragma once
#include "common.h"

#include "glutils.h"
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

struct ShaderProgram
{
    enum BufferBindingPoints
    {
        BIND_TRANSFORM
    };

    GLProgram glProgram = 0;
    GLUniformLocation baseColorLoc = -1;
    GLUniformLocation textureScaleLoc = -1;
};

class ShaderManager
{
public:
    void init();

    shared_ptr<ShaderProgram> coloredProg, texturedProg,
        shiftedTextureProg, tintedTextureProg;
    shared_ptr<ShaderProgram> debugProg;

private:
    GLShader compileShader(GLType type, string name,
        std::initializer_list<string> sources);
    GLProgram linkProgram(string name, std::initializer_list<GLShader> shaders);
    void setProgramBindings(ShaderProgram &program);

    GLShader basicVert = 0;
};


struct Texture
{
    static const shared_ptr<Texture> NO_TEXTURE;

    GLTexture glTexture = 0;
};

struct Material
{
    enum TextureUnits
    {
        TEXTURE_BASE
    };

    shared_ptr<ShaderProgram> shader;  // never null
    bool transparent = false;
    shared_ptr<Texture> texture;  // never null
    glm::vec4 color {1, 1, 1, 1};
// https://extensions.sketchup.com/developers/sketchup_c_api/sketchup/struct_s_u_texture_ref.html#ac9341c5de53bcc1a89e51de463bd54a0
    glm::vec2 scale {1, 1};
};

}  // namespace
