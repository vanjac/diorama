#pragma once
#include "common.h"

#include <GL/gl3w.h>
#include "material.h"

struct Primitive
{
    enum VertexAttributes
    {
        ATTRIB_POSITION, ATTRIB_NORMAL, ATTRIB_STQ, ATTRIB_MAX
    };

    GLuint glVertexArray = 0;
    GLuint glAttribBuffers[ATTRIB_MAX] {0};
    GLuint glElementBuffer = 0;
    GLsizei numIndices = 0;

    shared_ptr<Material> material;  // null for default material
};

struct Mesh
{
    vector<Primitive> primitives;
    // TODO edges
    // TODO bounds
};
