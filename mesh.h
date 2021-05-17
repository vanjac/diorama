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

    GLVertexArray glVertexArray = 0;
    GLBuffer glAttribBuffers[ATTRIB_MAX] {0};
    GLBuffer glElementBuffer = 0;
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
