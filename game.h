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

    void renderHierarchy(const Component &component,
        const Material *inherit, bool transparent);
    void renderPrimitive(const RenderPrimitive &primitive,
        const Material *inherit, bool transparent);
    void setTransform(const TransformBlock &block);
    void setMaterial(const Material *material, bool inherited);
    void setTexture(int unit, GLTexture texture);

    SDL_Window *window;
    World world;
    bool running = true;

    ShaderManager shaders;
    shared_ptr<Material> defaultMaterial;

    TransformBlock transform;
    GLBuffer transformUBO;  // shared between all programs

    float camYaw = 0, camPitch = 0;
    glm::vec3 camPos{0, 0, 0};
    glm::vec3 flyPos{0, 0, 0};
    glm::vec3 flyNeg{0, 0, 0};
    float flySpeed = 70.0f;  // inches per second
};

}  // namespace
