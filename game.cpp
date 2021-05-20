#include "game.h"
#include "load_skp.h"
#include <GL/gl3w.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace diorama {

const float PROJ_FOV = glm::radians(60.0f);
const float PROJ_NEAR = 5;
const float PROJ_FAR = 10000;

const float LOOK_SPEED = 0.007;
const float FLY_SPEED_ADJUST = 0.2f;

// remap opengl coordinates to blender coordinates
const glm::mat4 REMAP_AXES(
    glm::vec4(1,0,0,0),
    glm::vec4(0,0,-1,0),
    glm::vec4(0,1,0,0),
    glm::vec4(0,0,0,1));

Game::Game(SDL_Window *window)
    : window(window)
    , defaultMaterial(new Material)
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

    glClearColor(0, 0, 0, 1);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int winW, winH;
    SDL_GetWindowSize(window, &winW, &winH);
    resizeGL(winW, winH);

    // https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
    glGenBuffers(1, &transformUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, transformUBO);
    glBufferData(GL_UNIFORM_BUFFER,
        sizeof(TransformBlock), &transform, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER,
        ShaderProgram::BIND_TRANSFORM, transformUBO);

    shaders.init();
    defaultMaterial->shader = shaders.coloredProg;
    defaultMaterial->texture = Texture::NO_TEXTURE;

    {
        SkpLoader loader(path, shaders);
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
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    resizeGL(event.window.data1, event.window.data2);
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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Transform camTransform = Transform::rotate(camYaw, Transform::UP);
        camTransform *= Transform::rotate(camPitch, Transform::RIGHT);
        glm::vec3 flyVec = flyPos + flyNeg;
        if (flyVec != glm::vec3(0)) {
            flyVec = glm::normalize(flyVec);
            flyVec *= deltaTime * flySpeed;
        }
        camPos += camTransform.transformVector(flyVec);
        camTransform = Transform::translate(camPos) * camTransform;

        transform.ViewMatrix = camTransform.inverse().matrix();

        transform.ModelMatrix = glm::mat4(1);
        renderHierarchy(*world.root(), defaultMaterial.get(), false);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        transform.ModelMatrix = glm::mat4(1);
        renderHierarchy(*world.root(), defaultMaterial.get(), true);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        // TODO  ??
        //glFlush();
        SDL_GL_SwapWindow(window);
    }

    return EXIT_SUCCESS;
}


void Game::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    transform.ProjectionMatrix = glm::perspective(
        PROJ_FOV, (float)w / h, PROJ_NEAR, PROJ_FAR) * REMAP_AXES;
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


void Game::renderHierarchy(const Component &component,
                           const Material *inherit, bool transparent)
{
    // TODO make this faster, cache values, etc.
    if (component.material)
        inherit = component.material.get();
    glm::mat4 prevModel = transform.ModelMatrix;
    transform.ModelMatrix *= component.tLocal().matrix();
    if (component.mesh && !component.mesh->render.empty()) {
        glm::mat3 model3 = transform.ModelMatrix;
        transform.NormalMatrix = glm::transpose(glm::inverse(model3));
        // detect negative scale https://gamedev.stackexchange.com/a/54508
        // TODO determinant is computed twice
        bool reversed = glm::determinant(model3) < 0;
        if (reversed)
            glCullFace(GL_FRONT);

        // TODO bad to update for every component?
        glBindBuffer(GL_UNIFORM_BUFFER, transformUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(TransformBlock),
                        &transform);

        for (auto &primitive : component.mesh->render) {
            renderPrimitive(primitive, inherit, transparent);
        }

        if (reversed)
            glCullFace(GL_BACK);
    }
    for (auto &child : component.children()) {
        renderHierarchy(*child, inherit, transparent);
    }
    transform.ModelMatrix = prevModel;
}

void Game::renderPrimitive(const RenderPrimitive &primitive,
                           const Material *inherit, bool transparent)
{
    const Material *mat = primitive.material.get();
    if (!mat)
        mat = inherit;
    if (mat->transparent != transparent)
        return;

    glBindVertexArray(primitive.vertexArray);
    setMaterial(mat, mat == inherit);
    glDrawElements(GL_TRIANGLES, primitive.numIndices,
                   GL_UNSIGNED_SHORT, (void *)0);
}

void Game::setMaterial(const Material *material, bool inherited)
{
    glUseProgram(material->shader->glProgram);
    setTexture(Material::TEXTURE_BASE, material->texture->glTexture);
    glm::vec4 color = material->color;
    glUniform4fv(material->shader->baseColorLoc, 1, glm::value_ptr(color));
// https://extensions.sketchup.com/developers/sketchup_c_api/sketchup/struct_s_u_texture_ref.html#ac9341c5de53bcc1a89e51de463bd54a0
    glm::vec2 scale = inherited ? material->scale : glm::vec2(1, 1);
    glUniform2fv(material->shader->textureScaleLoc, 1, glm::value_ptr(scale));
}

void Game::setTexture(int unit, GLTexture texture)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

}  // namespace
