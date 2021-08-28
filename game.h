#pragma once
#include "common.h"

#include <SDL.h>
#include <glm/glm.hpp>
#include "glutils.h"
#include "world.h"

namespace diorama {

class Game
{
public:
    Game(SDL_Window *window);
    int main(const vector<string> args);

private:
    void resizeGL(int w, int h);

    void keyDown(const SDL_KeyboardEvent &e);
    void keyUp(const SDL_KeyboardEvent &e);

    void renderHierarchy(const Component &component, glm::mat4 modelMatrix,
        const Material *inherit, RenderOrder order);
    void renderPrimitive(const RenderPrimitive &primitive,
        glm::mat4 modelMatrix, glm::mat3 normalMatrix,
        const Material *inherit, RenderOrder order);
    void setCamera(const CameraBlock &block);
    void setMaterial(const Material *material, bool inherited);
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
