#pragma once
#include "common.h"

#include "world.h"

namespace diorama::physics {

struct CollisionInfo
{
    Component *component = nullptr; // null for no collision
    // calculating distances is problematic, do it yourself
    glm::vec3 point{0, 0, 0};
    glm::vec3 normal{0, 0, 0};
    // TODO substance
};

CollisionInfo raycast(const World &world, glm::vec3 origin, glm::vec3 dir);

}  // namespace
