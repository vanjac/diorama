#pragma once
#include "common.h"

#include "component.h"
#include <unordered_map>

namespace diorama {

class World {
public:
    shared_ptr<Component> root() const;
    void setRoot(shared_ptr<Component> root);

    // called by Component
    void addHierarchy(Component *component);
    void removeHierarchy(Component *component);

private:
    void addComponent(Component *component);
    void removeComponent(Component *component);

    shared_ptr<Component> _root;
};


}  // namespace
