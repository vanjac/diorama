#include "component.h"
#include "world.h"

namespace diorama {

Component::Component()
{}

Component::Component(const Component &other)
    : name(other.name)
    , mesh(other.mesh)
    , material(other.material)
    , _tLocal(other._tLocal)
{}

Component & Component::operator=(const Component &rhs)
{
    this->name = rhs.name;
    this->mesh = rhs.mesh;
    this->material = rhs.material;
    this->_tLocal = rhs._tLocal;
    return *this;
}

Component::~Component()
{
    for (auto &child : _children) {
        delete child;
    }
}

const Transform & Component::tLocal() const
{
    return _tLocal;
}

Transform & Component::tLocalMut()
{
    return _tLocal;
}

Component * Component::parent() const
{
    return _parent;
}

void Component::setParent(Component *parent)
{
    if (parent == _parent)
        return;
    if (_parent) {
        auto &childrenVec = _parent->_children;
        // TODO fast remove without preserving order
        auto childIt = std::find(childrenVec.begin(), childrenVec.end(), this);
        if (childIt != childrenVec.end())
            childrenVec.erase(childIt);
    }
    _parent = parent;
    if (parent) {
        parent->_children.push_back(this);
        setWorld(parent->world());
    } else {
        setWorld(nullptr);
    }
}

const vector<Component *> Component::children() const
{
    return _children;
}

Component * Component::cloneHierarchy() const
{
    Component *copy = new Component(*this);
    for (auto &child : _children) {
        Component *childCopy = child->cloneHierarchy();
        // should have no world so this should be fine
        childCopy->setParent(copy);
    }
    return copy;
}

World * Component::world() const
{
    return _world;
}

void Component::setWorld(World *world)
{
    if (world == _world)
        return;
    if (_world)
        _world->removeHierarchy(this);
    _world = world;
    if (world);
        world->addHierarchy(this);
}

}  // namespace
