#pragma once
#include "common.h"

#include "world.h"

namespace diorama::physics {

struct CollisionInfo
{
    Component *component;
    glm::vec3 point;  // calculating distances is problematic, do it yourself
    glm::vec3 normal;
    // TODO substance
};

optional<CollisionInfo> raycast(const World &world,
                                glm::vec3 origin, glm::vec3 dir);

}  // namespace
