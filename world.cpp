#include "world.h"

namespace diorama {

shared_ptr<Component> World::root() const
{
    return _root;
}

void World::setRoot(shared_ptr<Component> root)
{
    if (_root == root)
        return;
    if (_root) {
        _root->setWorld(nullptr);
    }
    _root = root;
    if (root) {
        _root->setWorld(this);
    }
}

void World::addComponent(Component *component)
{
    printf("add component %s\n", component->name.c_str());  // TODO
}

void World::removeComponent(Component *component)
{
    printf("remove component %s\n", component->name.c_str());  // TODO
}

void World::addHierarchy(Component *component)
{
    addComponent(component);
    for (auto &child : component->children()) {
        addHierarchy(child.get());
    }
}

void World::removeHierarchy(Component *component)
{
    removeComponent(component);
    for (auto &child : component->children()) {
        removeHierarchy(child.get());
    }
}

}  // namespace
