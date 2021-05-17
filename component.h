#pragma once
#include "common.h"

#include "mathutils.h"
#include "mesh.h"
#include "material.h"

namespace diorama {

class World;

// should always be owned by a shared_ptr (never on the stack)
class Component
    : public std::enable_shared_from_this<Component>
{
public:
    Component();
    // does not copy hierarchy, use cloneHierarchy for that
    // does not copy world
    Component(const Component &other);
    Component & operator=(const Component &rhs);

    string name;  // shouldn't change after adding to world
    shared_ptr<Mesh> mesh;  // could be null
    // overrides defaults in mesh and children. null for default
    shared_ptr<Material> material;

    const Transform & local() const;
    Transform & localMut();

    Component * parent() const;
    void setParent(Component *parent);
    const vector<shared_ptr<Component>> children() const;

    shared_ptr<Component> cloneHierarchy();

    World * world() const;
    void setWorld(World *world);  // called by World

private:
    Transform _local;
    // TODO cache world matrix

    Component *_parent = nullptr;  // instead of weak_ptr
    vector<shared_ptr<Component>> _children;

    World *_world = nullptr;
};

}  // namespace
