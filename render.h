#pragma once

#include <glm/glm.hpp>
#include "common.h"
#include "world.h"
#include "glutils.h"

namespace diorama::render {

// https://realtimecollisiondetection.net/blog/?p=86
// https://blog.molecular-matters.com/2014/11/06/stateless-layered-multi-threaded-rendering-part-1/
struct DrawCall
{
    uint32_t sortKey;
    const RenderPrimitive *primitive;
    const Material *material;
    glm::mat4 modelMatrix;
    glm::mat3 normalMatrix;
    bool reversed;  // cull front faces instead of back faces
    bool textureScale;  // apply material texture scale

    bool operator<(const DrawCall &rhs) const;
};

class Renderer
{
public:
    Renderer(const ShaderManager &shaders);

    void resize(int w, int h);
    void render(const World *world, const Transform &camTransform);

    void debugLine(glm::vec3 start, glm::vec3 end, glm::vec3 color);

private:

    void drawHierarchy(vector<DrawCall> &drawCalls, const Component *component,
                       glm::mat4 cameraMatrix, glm::mat4 modelMatrix,
                       const Material *inherit);
    void computeSortKey(DrawCall *call, glm::mat4 cameraMatrix);
    void renderDrawCalls(const vector<DrawCall> &drawCalls);

    void setTexture(int unit, GLTexture texture);
    void setTransform(const ShaderProgram *shader,
                      glm::mat4 modelMatrix, glm::mat3 normalMatrix);

    const ShaderManager &shaders;

    Material defaultMaterial;

    glm::mat4 projectionMatrix;
    GLBuffer cameraUBO;  // shared between all programs

    vector<DrawCall> drawCalls;  // avoid reconstructing vector each frame

    GLVertexArray debugVertexArray;
    GLBuffer debugVertexBuffer;
};

}  // namespace