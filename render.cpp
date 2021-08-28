#include "render.h"
#include <algorithm>
#include <GL/gl3w.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace diorama::render {

// remap opengl coordinates to blender coordinates
const glm::mat4 REMAP_AXES(
    glm::vec4(1,0,0,0),
    glm::vec4(0,0,-1,0),
    glm::vec4(0,1,0,0),
    glm::vec4(0,0,0,1));

bool DrawCall::operator<(const DrawCall &rhs) const
{
    return sortKey < rhs.sortKey;
}

Renderer::Renderer(const ShaderManager *shaders)
{
    glClearColor(0, 0, 0, 1);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
    glGenBuffers(1, &cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    CameraBlock initCameraBlock;
    glBufferData(GL_UNIFORM_BUFFER,
        sizeof(CameraBlock), &initCameraBlock, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER,
        ShaderProgram::BIND_TRANSFORM, cameraUBO);

    defaultMaterial.shader = &shaders->coloredProg;
    defaultMaterial.texture = &Texture::NO_TEXTURE;
    debugShader = &shaders->debugProg;

    glGenVertexArrays(1, &debugVertexArray);
    glBindVertexArray(debugVertexArray);
    glGenBuffers(1, &debugVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, debugVertexBuffer);
    glm::vec3 initVertices[] { {0,0,0}, {0,0,0} };
    glBufferData(GL_ARRAY_BUFFER, sizeof(initVertices), initVertices,
                 GL_STREAM_DRAW);
    glVertexAttribPointer(RenderPrimitive::ATTRIB_POSITION, 3, GL_FLOAT,
                          GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(RenderPrimitive::ATTRIB_POSITION);
    glBindVertexArray(0);
}

void Renderer::setCameraParameters(float fov, float nearClip, float farClip)
{
    this->cameraFOV = fov;
    this->nearClip = nearClip;
    this->farClip = farClip;
    updateProjectionMatrix();
}

void Renderer::resizeWindow(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    updateProjectionMatrix();
}

void Renderer::updateProjectionMatrix()
{
    projectionMatrix = glm::perspective(cameraFOV,
        (float)windowWidth / windowHeight, nearClip, farClip) * REMAP_AXES;
}

void Renderer::render(const World *world, const Transform &camTransform)
{
    glm::mat4 viewMatrix = camTransform.inverse().matrix();
    glm::mat4 cameraMatrix = projectionMatrix * viewMatrix;
    CameraBlock cameraBlock {viewMatrix, projectionMatrix};
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraBlock), &cameraBlock);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    drawCalls.clear();
    drawHierarchy(drawCalls, world->root(), cameraMatrix, glm::mat4(1),
                    &defaultMaterial);
    std::sort(drawCalls.begin(), drawCalls.end());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderDrawCalls(drawCalls);

    // TODO glFlush?
}

void Renderer::drawHierarchy(vector<DrawCall> &drawCalls,
                         const Component *component,
                         glm::mat4 cameraMatrix, glm::mat4 modelMatrix,
                         const Material *inherit)
{
    // TODO make this faster, cache values, etc.
    if (component->material)
        inherit = component->material;
    modelMatrix *= component->tLocal().matrix();
    if (component->mesh && !component->mesh->render.empty()) {
        glm::mat3 model3 = modelMatrix;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(model3));
        // detect negative scale https://gamedev.stackexchange.com/a/54508
        // TODO determinant is computed twice
        bool reversed = glm::determinant(model3) < 0;

        for (auto &primitive : component->mesh->render) {
            const Material *material = primitive.material;
            DrawCall call {
                0,
                &primitive,
                material ? material : inherit,
                modelMatrix,
                normalMatrix,
                reversed,
// https://extensions.sketchup.com/developers/sketchup_c_api/sketchup/struct_s_u_texture_ref.html#ac9341c5de53bcc1a89e51de463bd54a0
                !material
            };
            computeSortKey(&call, cameraMatrix);
            drawCalls.push_back(call);
        }
    }
    for (auto &child : component->children()) {
        drawHierarchy(drawCalls, child, cameraMatrix, modelMatrix, inherit);
    }
}

void Renderer::computeSortKey(DrawCall *call, glm::mat4 cameraMatrix) {
    call->sortKey = 0;
    // 30 - 31: render order
    call->sortKey |= (uint32_t)(call->material->order) << 30;
    // 14 - 29: depth
    if (call->material->order == RenderOrder::Transparent) {
        // https://community.khronos.org/t/projection-matrix-mapping-the-z/46938
        glm::vec4 gl_Position = (cameraMatrix * call->modelMatrix)[3];
        // TODO range seems biased towards high values
        float depth = glm::clamp(gl_Position.z / gl_Position.w, -1.0f, 1.0f);
        uint16_t depthInt;
        if (depth >= 1.0f || depth <= -1.0f) // could be behind the camera
            depthInt = -1;
        else
            depthInt = (uint16_t)((-depth + 1) / 2 * (float)(1<<16));
        call->sortKey |= (uint32_t)depthInt << 14;
    }
    // 9 - 13: shader
    call->sortKey |= (call->material->shader->glProgram & 0x1F) << 9;
    // 0 - 8: material
    // https://stackoverflow.com/q/20953390
    static const size_t shift = (size_t)log2(1 + sizeof(Material));
    size_t matPtr = (size_t)call->material;
    call->sortKey |= ((matPtr >> shift) ^ (matPtr >> (shift + 9))) & 0x1FF;
}

void Renderer::renderDrawCalls(const vector<DrawCall> &drawCalls)
{
    const Material *curMaterial = nullptr;
    const ShaderProgram *curShader = nullptr;

    // TODO is it actually necessary to avoid gl state changes?
    RenderOrder curOrder = RenderOrder::Opaque;
    bool curReversed = false;
    // init gl state
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    for (auto &call : drawCalls) {
        if (call.material != curMaterial) {
            curMaterial = call.material;

            if (curMaterial->shader != curShader) {
                curShader = call.material->shader;
                glUseProgram(curShader->glProgram);
            }

            setTexture(Material::TEXTURE_BASE, curMaterial->texture->glTexture);
            glm::vec4 color = curMaterial->color;
            glUniform4fv(curShader->baseColorLoc, 1, glm::value_ptr(color));

            if (curMaterial->order != curOrder) {
                curOrder = curMaterial->order;
                switch (curOrder) {
                case RenderOrder::Transparent:
                    glEnable(GL_BLEND);
                    glDepthMask(GL_FALSE);
                    break;
                case RenderOrder::Opaque:
                    glDisable(GL_BLEND);
                    glDepthMask(GL_TRUE);
                    break;
                }
            }
        }

        if (call.reversed != curReversed) {
            curReversed = call.reversed;
            glCullFace(curReversed ? GL_FRONT : GL_BACK);
        }

        // set uniforms
        setTransform(curShader, call.modelMatrix, call.normalMatrix);
        glm::vec2 scale = call.textureScale ? curMaterial->scale
            : glm::vec2(1, 1);
        // TODO reduce calls? only necessary when texture is set
        glUniform2fv(curShader->textureScaleLoc, 1, glm::value_ptr(scale));

        glBindVertexArray(call.primitive->vertexArray);
        glDrawElements(GL_TRIANGLES, call.primitive->numIndices,
                       GL_UNSIGNED_SHORT, (void *)0);
    }

    // reset gl state
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glUseProgram(0);
    glBindVertexArray(0);
}

void Renderer::setTexture(int unit, GLTexture texture)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

void Renderer::setTransform(const ShaderProgram *shader,
                        glm::mat4 modelMatrix, glm::mat3 normalMatrix)
{
    glUniformMatrix4fv(shader->modelMatrixLoc, 1, GL_FALSE,
                       glm::value_ptr(modelMatrix));
    glUniformMatrix3fv(shader->normalMatrixLoc, 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));
}

void Renderer::debugLine(glm::vec3 start, glm::vec3 end, glm::vec3 color)
{
    glm::vec3 data[] {start, end};
    glBindVertexArray(debugVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, debugVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);

    glUseProgram(debugShader->glProgram);
    glm::vec4 color4(color, 1);
    glUniform4fv(debugShader->baseColorLoc, 1, glm::value_ptr(color4));
    setTransform(debugShader, glm::mat4(1), glm::mat3(1));

    glDrawArrays(GL_LINES, 0, 2);

    glUseProgram(0);
    glBindVertexArray(0);
}

}  // namespace