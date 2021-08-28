#include "game.h"
#include "load_skp.h"
#include <cstdio>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>

namespace diorama {

const float LOOK_SPEED = 0.007;
const float FLY_SPEED_ADJUST = 0.2f;

Game::Game(SDL_Window *window)
    : window(window)
    , renderer(&shaders)
{}

int Game::main(const vector<string> args)
{
    if (args.size() < 2) {
        printf("please specify a path\n");
        return EXIT_FAILURE;
    }
    string path = args[1];
    if (path.compare(path.length() - 4, 4, ".skb") == 0) {
        printf("that's a backup file! look for .skp extension instead\n");
        return EXIT_FAILURE;
    }

    int winW, winH;
    SDL_GetWindowSize(window, &winW, &winH);
    renderer.resizeWindow(winW, winH);

    {
        SkpLoader loader(path, &world, &shaders);
        loader.loadGlobal();
        world.setRoot(loader.loadRoot());
    }

    int startTick = SDL_GetTicks();
    int prevTick = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    renderer.resizeWindow(
                        event.window.data1, event.window.data2);
                }
                break;
            case SDL_KEYDOWN:
                keyDown(event.key);
                break;
            case SDL_KEYUP:
                keyUp(event.key);
                break;
            case SDL_MOUSEMOTION:
                camYaw -= event.motion.xrel * LOOK_SPEED;
                camPitch -= event.motion.yrel * LOOK_SPEED;
                camPitch = glm::clamp(camPitch,
                    -glm::pi<float>() / 2, glm::pi<float>() / 2);
                break;
            case SDL_MOUSEWHEEL:
                flySpeed *= glm::exp(event.wheel.y * FLY_SPEED_ADJUST);
                break;
            case SDL_QUIT:
                running = false;
                break;
            }
        }
        int tick = SDL_GetTicks() - startTick;
        float time = tick / 1000.0f;
        float deltaTime = (tick - prevTick) / 1000.0f;
        prevTick = tick;

        Transform camTransform = Transform::rotate(camYaw, Transform::UP);
        camTransform *= Transform::rotate(camPitch, Transform::RIGHT);
        glm::vec3 flyVec = flyPos + flyNeg;
        if (flyVec != glm::vec3(0)) {
            flyVec = glm::normalize(flyVec);
            flyVec *= deltaTime * flySpeed;
        }
        camPos += camTransform.transformVector(flyVec);
        camTransform = Transform::translate(camPos) * camTransform;

        renderer.render(&world, camTransform);

        SDL_GL_SwapWindow(window);
    }

    return EXIT_SUCCESS;
}

void Game::keyDown(const SDL_KeyboardEvent &e)
{
    switch(e.keysym.sym) {
    case SDLK_d:
        flyPos.x = 1;   break;
    case SDLK_a:
        flyNeg.x = -1;  break;
    case SDLK_w:
        flyPos.y = 1;   break;
    case SDLK_s:
        flyNeg.y = -1;  break;
    case SDLK_e:
        flyPos.z = 1;   break;
    case SDLK_q:
        flyNeg.z = -1;  break;
    }
}

void Game::keyUp(const SDL_KeyboardEvent &e)
{
    switch(e.keysym.sym) {
    case SDLK_d:
        flyPos.x = 0;   break;
    case SDLK_a:
        flyNeg.x = 0;  break;
    case SDLK_w:
        flyPos.y = 0;   break;
    case SDLK_s:
        flyNeg.y = 0;  break;
    case SDLK_e:
        flyPos.z = 0;   break;
    case SDLK_q:
        flyNeg.z = 0;  break;
    }
}

}  // namespace
