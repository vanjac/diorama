#pragma once
#include "common.h"

#include "render.h"
#include "collision.h"
#include "world.h"
#include <glm/glm.hpp>
#include <SDL.h>

namespace diorama {

class Game
{
public:
    Game(SDL_Window *window);
    void main(const vector<string> args);

private:
    void keyDown(const SDL_KeyboardEvent &e);
    void keyUp(const SDL_KeyboardEvent &e);

    SDL_Window *window;
    World world;
    bool running = true;

    render::Renderer renderer;
    ShaderManager shaders;

    float camYaw = 0, camPitch = 0;
    glm::vec3 camPos{0, 0, 128};
    glm::vec3 flyPos{0, 0, 0};
    glm::vec3 flyNeg{0, 0, 0};
    float flySpeed = 70.0f;  // inches per second

    vector<physics::CollisionInfo> playerCollisions;
};

}  // namespace
