#include "component.h"
#include "world.h"

namespace diorama {

Component::Component()
{}

Component::Component(Component const &other)
    : name(other.name)
    , mesh(other.mesh)
    , material(other.material)
    , _tLocal(other._tLocal)
{}

Component & Component::operator=(Component const &rhs)
{
    this->name = rhs.name;
    this->mesh = rhs.mesh;
    this->material = rhs.material;
    this->_tLocal = rhs._tLocal;
    return *this;
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
    shared_ptr<Component> sharedThis = shared_from_this();
    if (_parent) {
        auto &childrenVec = _parent->_children;
        // TODO fast remove without preserving order
        auto childIt = std::find(childrenVec.begin(), childrenVec.end(),
                                 sharedThis);
        if (childIt != childrenVec.end())
            childrenVec.erase(childIt);
    }
    _parent = parent;
    if (parent) {
        parent->_children.push_back(sharedThis);
        setWorld(parent->world());
    } else {
        setWorld(nullptr);
    }
}

const vector<shared_ptr<Component>> Component::children() const
{
    return _children;
}

shared_ptr<Component> Component::cloneHierarchy()
{
    shared_ptr<Component> copy(new Component(*this));
    for (auto &child : _children) {
        shared_ptr<Component> childCopy = child->cloneHierarchy();
        // should have no world so this should be fine
        childCopy->setParent(copy.get());
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
