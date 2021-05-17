#pragma once
#include "common.h"

#include "glutils.h"
#include "material.h"

namespace diorama {

struct Primitive
{
    enum VertexAttributes
    {
        ATTRIB_POSITION, ATTRIB_NORMAL, ATTRIB_STQ, ATTRIB_MAX
    };

    GLVertexArray vertexArray = 0;
    GLBuffer attribBuffers[ATTRIB_MAX] {0};
    GLBuffer elementBuffer = 0;
    int numIndices = 0;

    shared_ptr<Material> material;  // null for default material
};

struct Mesh
{
    vector<Primitive> primitives;
    // TODO edges
    // TODO bounds
};

}  // namespace
