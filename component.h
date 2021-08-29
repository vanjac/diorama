#pragma once
#include "common.h"

#include "material.h"
#include "mathutils.h"
#include "mesh.h"

namespace diorama {

class World;

class Component
{
public:
    Component();
    // does not copy hierarchy, use cloneHierarchy for that
    // does not copy world
    Component(const Component &other);
    Component & operator=(const Component &rhs);
    ~Component();

    string name;  // shouldn't change after adding to world
    const Mesh *mesh = nullptr;  // could be null
    // overrides defaults in mesh and children. null for default
    const Material *material = nullptr;

    const Transform & tLocal() const;
    Transform & tLocalMut();

    Component * parent() const;
    // parent takes ownership of child
    void setParent(Component *parent);
    const vector<Component *> children() const;

    Component * cloneHierarchy();

    World * world() const;
    void setWorld(World *world);  // called by World

private:
    Transform _tLocal;
    // TODO cache world matrix

    Component *_parent = nullptr;  // instead of weak_ptr
    vector<Component *> _children;

    World *_world = nullptr;
};

}  // namespace
