#pragma once

#include <vector>
#include <memory>
#include "mathutils.h"
#include "mesh.h"
#include "material.h"

// should always be owned by a shared_ptr (never on the stack)
class Component
    : public std::enable_shared_from_this<Component>
{
public:
    Component();
    // does not copy hierarchy, use cloneHierarchy for that
    Component(const Component &other);
    Component & operator=(const Component &rhs);

    std::string name;
    std::shared_ptr<Mesh> mesh;  // could be null
    // overrides defaults in mesh and children. null for default
    std::shared_ptr<Material> material;

    const Transform & local() const;
    Transform & localMut();

    Component * parent() const;
    void setParent(Component *parent);
    const std::vector<std::shared_ptr<Component>> children() const;

    std::shared_ptr<Component> cloneHierarchy();

private:
    Transform _local;
    // TODO cache world matrix

    Component *_parent = nullptr;  // instead of weak_ptr
    std::vector<std::shared_ptr<Component>> _children;
};
