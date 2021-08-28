#pragma once
#include "common.h"

#include <SDL.h>
#include <glm/glm.hpp>
#include "world.h"
#include "render.h"

namespace diorama {

class Game
{
public:
    Game(SDL_Window *window);
    int main(const vector<string> args);

private:
    void keyDown(const SDL_KeyboardEvent &e);
    void keyUp(const SDL_KeyboardEvent &e);

    SDL_Window *window;
    World world;
    bool running = true;

    render::Renderer renderer;
    ShaderManager shaders;

    float camYaw = 0, camPitch = 0;
    glm::vec3 camPos{0, 0, 0};
    glm::vec3 flyPos{0, 0, 0};
    glm::vec3 flyNeg{0, 0, 0};
    float flySpeed = 70.0f;  // inches per second
};

}  // namespace
