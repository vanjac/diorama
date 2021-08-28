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

void RenderPrimitive::setAttribData(VertexAttribute attrib, size_t size,
    int components, GLDataType type, const void *data)
{
    // array buffer bindings are not stored in VAO
    // https://gamedev.stackexchange.com/a/99238
    // https://stackoverflow.com/a/26559063
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, attribBuffers[attrib]);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glVertexAttribPointer(attrib, components, (GLenum)type,
                          GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(attrib);
    glBindVertexArray(0);
}

void RenderPrimitive::setIndices(int numIndices, const MeshIndex *indices)
{
    glBindVertexArray(vertexArray);
    // element buffer binding *is* stored in VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(MeshIndex),
                 indices, GL_STATIC_DRAW);
    glBindVertexArray(0);
    this->numIndices = numIndices;
}

}  // namespace
