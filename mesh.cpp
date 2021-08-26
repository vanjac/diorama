#include "mesh.h"
#include <GL/gl3w.h>

namespace diorama {

RenderPrimitive::RenderPrimitive()
{
    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(ATTRIB_MAX, attribBuffers.data());
    glGenBuffers(1, &elementBuffer);
}

RenderPrimitive::~RenderPrimitive()
{
    if (vertexArray != 0) {
        glDeleteVertexArrays(1, &vertexArray);
        glDeleteBuffers(ATTRIB_MAX, attribBuffers.data());
        glDeleteBuffers(1, &elementBuffer);
    }
}

RenderPrimitive::RenderPrimitive(RenderPrimitive &&other)
    : vertexArray(other.vertexArray)
    , attribBuffers(other.attribBuffers)
    , elementBuffer(other.elementBuffer)
    , numIndices(other.numIndices)
    , material(other.material)
{
    other.vertexArray = 0;
    other.attribBuffers.fill(0);
    other.elementBuffer = 0;
    other.material = nullptr;
}

}  // namespace
