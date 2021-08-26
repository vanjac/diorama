#pragma once
#include "common.h"

#include "glutils.h"
#include "resource.h"
#include "material.h"

namespace diorama {

using MeshIndex = uint16_t;

class RenderPrimitive : noncopyable
{
public:
    enum VertexAttributes
    {
        ATTRIB_POSITION, ATTRIB_NORMAL, ATTRIB_STQ, ATTRIB_MAX
    };

    RenderPrimitive();
    ~RenderPrimitive();
    RenderPrimitive(RenderPrimitive &&other);

    GLVertexArray vertexArray;
    array<GLBuffer, ATTRIB_MAX> attribBuffers;  // buffers for vertex attributes
    GLBuffer elementBuffer;  // buffer for element indices
    int numIndices = 0;

    Material *material = nullptr;  // null for default material
};

struct CollisionPrimitive
{
    vector<glm::vec3> vertices;
    vector<MeshIndex> indices;  // triangles
    // TODO substance
};

struct Mesh : Resource
{
    vector<RenderPrimitive> render;
    vector<CollisionPrimitive> collision;
    // TODO edges
    // TODO bounds
};

}  // namespace
