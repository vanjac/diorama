#pragma once
#include "common.h"

#include <SDL.h>
#include <glm/glm.hpp>
#include "glutils.h"
#include "world.h"

namespace diorama {

// https://realtimecollisiondetection.net/blog/?p=86
// https://blog.molecular-matters.com/2014/11/06/stateless-layered-multi-threaded-rendering-part-1/
struct DrawCall
{
    uint32_t sortKey;
    const RenderPrimitive *primitive;
    const Material *material;
    glm::mat4 modelMatrix;
    glm::mat3 normalMatrix;
    bool reversed; // cull front faces instead of back faces
    bool noTextureScale; // ignore material texture scale

    bool operator<(const DrawCall &rhs) const;
};

class Game
{
public:
    Game(SDL_Window *window);
    int main(const vector<string> args);

private:
    void resizeGL(int w, int h);

    void keyDown(const SDL_KeyboardEvent &e);
    void keyUp(const SDL_KeyboardEvent &e);

    void drawHierarchy(vector<DrawCall> &drawCalls, const Component &component,
                       glm::mat4 cameraMatrix, glm::mat4 modelMatrix,
                       const Material *inherit);
    void computeSortKey(DrawCall *call, glm::mat4 cameraMatrix);
    
    void render(const vector<DrawCall> &drawCalls);

    void setCamera(const CameraBlock &block);
    void setTexture(int unit, GLTexture texture);
    void setTransform(const ShaderProgram *shader,
                      glm::mat4 modelMatrix, glm::mat3 normalMatrix);

    void debugLine(glm::vec3 start, glm::vec3 end, glm::vec3 color);

    SDL_Window *window;
    World world;
    bool running = true;

    ShaderManager shaders;
    Material defaultMaterial;

    CameraBlock cameraBlock;
    GLBuffer cameraUBO;  // shared between all programs

    GLVertexArray debugVertexArray;
    GLBuffer debugVertexBuffer;

    float camYaw = 0, camPitch = 0;
    glm::vec3 camPos{0, 0, 0};
    glm::vec3 flyPos{0, 0, 0};
    glm::vec3 flyNeg{0, 0, 0};
    float flySpeed = 70.0f;  // inches per second
};

}  // namespace
