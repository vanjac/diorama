#pragma once

#include <memory>
#include <string>
#include <initializer_list>
#include <GL/gl3w.h>
#include <glm/glm.hpp>

// matches interface block in shader
// TODO: split off Model and Normal?
struct TransformBlock
{
    glm::mat4 ModelMatrix;
    glm::mat4 NormalMatrix;  // actually mat3
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
};

struct ShaderProgram
{
    enum BufferBindingPoints
    {
        BIND_TRANSFORM
    };

    GLuint glProgram = 0;
    GLint baseColorLoc = -1;
    GLint textureScaleLoc = -1;
};

class ShaderManager
{
public:
    void init();

    std::shared_ptr<ShaderProgram> coloredProg, texturedProg,
        shiftedTextureProg, tintedTextureProg;

private:
    GLuint compileShader(GLenum type, std::string name,
        std::initializer_list<std::string> sources);
    GLuint linkProgram(std::string name, std::initializer_list<GLuint> shaders);
    void setProgramBindings(ShaderProgram &program);
    
    GLuint basicVert = 0;
};


struct Texture
{
    static const std::shared_ptr<Texture> NO_TEXTURE;

    GLuint glTexture = 0;
};

struct Material
{
    enum TextureUnits
    {
        TEXTURE_BASE
    };

    std::shared_ptr<ShaderProgram> shader;  // never null
    bool transparent = false;
    std::shared_ptr<Texture> texture;  // never null
    glm::vec4 color {1, 1, 1, 1};
// https://extensions.sketchup.com/developers/sketchup_c_api/sketchup/struct_s_u_texture_ref.html#ac9341c5de53bcc1a89e51de463bd54a0
    glm::vec2 scale {1, 1};
};
