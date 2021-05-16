#include "component.h"

Component::Component()
{}

Component::Component(Component const &other)
    : name(other.name)
    , mesh(other.mesh)
    , material(other.material)
    , _local(other._local)
{}

Component & Component::operator=(Component const &rhs)
{
    this->name = rhs.name;
    this->mesh = rhs.mesh;
    this->material = rhs.material;
    this->_local = rhs._local;
    return *this;
}

const Transform & Component::local() const
{
    return _local;
}

Transform & Component::localMut()
{
    return _local;
}

Component * Component::parent() const
{
    return _parent;
}

void Component::setParent(Component *parent)
{
    std::shared_ptr<Component> sharedThis = shared_from_this();
    if (parent == _parent)
        return;
    if (_parent) {
        auto &childrenVec = _parent->_children;
        // TODO fast remove without preserving order
        auto childIt = std::find(childrenVec.begin(), childrenVec.end(), sharedThis);
        if (childIt != childrenVec.end())
            childrenVec.erase(childIt);
    }
    _parent = parent;
    if (parent)
        parent->_children.push_back(sharedThis);
}

const std::vector<std::shared_ptr<Component>> Component::children() const
{
    return _children;
}

std::shared_ptr<Component> Component::cloneHierarchy()
{
    std::shared_ptr<Component> copy(new Component(*this));
    for (auto &child : _children) {
        std::shared_ptr<Component> childCopy = child->cloneHierarchy();
        childCopy->setParent(copy.get());
    }
    return copy;
}