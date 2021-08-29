#include "world.h"

namespace diorama {

void World::addResource(const Resource *resource)
{
    _resources.emplace_back(resource);
}

Component * World::root() const
{
    return _root.get();
}

void World::setRoot(Component *root)
{
    if (_root.get() == root)
        return;
    if (_root) {
        _root->setWorld(nullptr);
    }
    _root = unique_ptr<Component>(root);
    if (root) {
        _root->setWorld(this);
    }
}

void World::addComponent(Component *component)
{
    cout << "add component " <<component->name<< "\n";  // TODO
    names[component->name].push_back(component);
}

void World::removeComponent(Component *component)
{
    cout << "remove component " <<component->name<< "\n";  // TODO
    auto &nameVec = names[component->name];
    auto compIt = std::find(nameVec.begin(), nameVec.end(), component);
    if (compIt != nameVec.end())
        nameVec.erase(compIt);
}

void World::addHierarchy(Component *component)
{
    addComponent(component);
    for (auto &child : component->children()) {
        addHierarchy(child);
    }
}

void World::removeHierarchy(Component *component)
{
    removeComponent(component);
    for (auto &child : component->children()) {
        removeHierarchy(child);
    }
}

Component * World::findComponent(string glob) const
{
    findComponents(glob, [](Component &c){
        return &c;
    });
    return nullptr;
}

}  // namespace
