#pragma once
#include "common.h"

#include "glutils.h"
#include "resource.h"
#include "material.h"

namespace diorama {

using MeshIndex = uint16_t;

struct RenderPrimitive
{
    enum VertexAttributes
    {
        ATTRIB_POSITION, ATTRIB_NORMAL, ATTRIB_STQ, ATTRIB_MAX
    };

    GLVertexArray vertexArray = 0;
    GLBuffer attribBuffers[ATTRIB_MAX] {0};
    GLBuffer elementBuffer = 0;
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
