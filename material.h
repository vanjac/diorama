#pragma once
#include "common.h"

#include "glutils.h"
#include "resource.h"
#include <glm/glm.hpp>

namespace diorama {

// matches interface block in shader
struct CameraBlock
{
    glm::mat4 ViewMatrix {1};
    glm::mat4 ProjectionMatrix {1};
};

class ShaderProgram : public Resource
{
public:
    enum BufferBindingPoint
    {
        BIND_TRANSFORM
    };

    ShaderProgram();
    ~ShaderProgram();

    void link(string name, initializer_list<GLShader> shaders);

    GLProgram glProgram;
    GLUniformLocation modelMatrixLoc = -1;
    GLUniformLocation normalMatrixLoc = -1;
    GLUniformLocation baseColorLoc = -1;
    GLUniformLocation textureScaleLoc = -1;
};

class ShaderManager
{
public:
    ShaderProgram coloredProg;
    ShaderProgram texturedProg;
    ShaderProgram shiftedTextureProg;
    ShaderProgram tintedTextureProg;
    ShaderProgram debugProg;

    void linkPrograms();

private:
    GLShader compileShader(GLShaderType type, string name,
        initializer_list<string> sources);

    GLShader basicVert = 0;
};

class Texture : public Resource
{
public:
    static const Texture NO_TEXTURE;

    Texture();
    Texture(GLTexture glTexture);
    ~Texture();

    void setImage(int width, int height, GLTextureFormat format,
                  GLDataType type, const void *data);

    GLTexture glTexture;
};

enum class RenderOrder
{
    Opaque, Transparent // TODO add cutout
};

struct Material : public Resource
{
    enum TextureUnit
    {
        TEXTURE_BASE
    };

    const ShaderProgram *shader = nullptr;  // never null
    RenderOrder order = RenderOrder::Opaque;
    const Texture *texture = nullptr;  // never null
    glm::vec4 color {1, 1, 1, 1};
// https://extensions.sketchup.com/developers/sketchup_c_api/sketchup/struct_s_u_texture_ref.html#ac9341c5de53bcc1a89e51de463bd54a0
    glm::vec2 scale {1, 1};
};

}  // namespace
