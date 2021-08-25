#pragma once
#include "common.h"

#include "component.h"
#include <unordered_map>

namespace diorama {

class World {
public:
    // takes ownership
    void addResource(Resource *resource);

    Component * root() const;
    // takes ownership
    void setRoot(Component *root);

    // called by Component
    void addHierarchy(Component *component);
    void removeHierarchy(Component *component);

    Component * findComponent(string glob) const;

    // functor should have the form f(Component &)
    template<typename Functor>
    bool findComponents(string glob, Functor f) const
    {
        // TODO support wildcards
        bool found = false;
        auto vecIt = names.find(glob);
        if (vecIt != names.end()) {
            for (auto component : vecIt->second) {
                found = true;
                f(*component);
            }
        }
        return found;
    }

private:
    void addComponent(Component *component);
    void removeComponent(Component *component);

    vector<unique_ptr<Resource>> _resources;

    unique_ptr<Component> _root;

    // map component name to list of components with that name
    std::unordered_map<string, vector<Component *>> names;
};


}  // namespace
