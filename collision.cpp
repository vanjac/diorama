#include "collision.h"
#include <glm/gtx/norm.hpp>
#include <limits>

namespace diorama::physics {

static optional<CollisionInfo> raycastHierarchy(
    Component &component, glm::vec3 origin, glm::vec3 dir);
// dir must be a unit vector
static optional<CollisionInfo> raycastPrimitive(
    const CollisionPrimitive &primitive, glm::vec3 origin, glm::vec3 dir,
    float *closestDist2);

optional<CollisionInfo> raycast(const World &world,
                                glm::vec3 origin, glm::vec3 dir)
{
    auto collision = raycastHierarchy(*world.root(), origin, dir);
    if (collision)
        collision->normal = glm::normalize(collision->normal);
    return collision;
}

static optional<CollisionInfo> raycastHierarchy(
    Component &component, glm::vec3 origin, glm::vec3 dir)
{
    const Transform &t = component.tLocal();
    Transform invT = t.inverse();
    origin = invT.transformPoint(origin);
    dir = invT.transformVector(dir);  // TODO preserve length

    float closestDist2 = std::numeric_limits<float>::max();
    optional<CollisionInfo> closest;

    if (component.mesh && !component.mesh->collision.empty()) {
        // dir may not be a unit vector at this point
        dir = glm::normalize(dir);
        for (auto &primitive : component.mesh->collision) {
            auto collision = raycastPrimitive(primitive, origin, dir,
                                              &closestDist2);
            if (collision) {
                closest = collision;
                closest->component = &component;
            }
        }
    }
    for (auto &child : component.children()) {
        auto collision = raycastHierarchy(*child, origin, dir);
        if (collision) {
            float dist2 = glm::distance2(origin, collision->point);
            if (dist2 < closestDist2) {
                closestDist2 = dist2;
                closest = collision;
            }
        }
    }

    if (closest) {
        closest->point = t.transformPoint(closest->point);
        closest->normal = glm::transpose(glm::mat3(invT.matrix()))
            * closest->normal;
        return closest;
    }
    return std::nullopt;
}

static optional<CollisionInfo> raycastPrimitive(
    const CollisionPrimitive &primitive, glm::vec3 origin, glm::vec3 dir,
    float *closestDist2)
{
    optional<CollisionInfo> closest;

    for (int i = 0; i < primitive.indices.size(); i += 3) {
        glm::vec3 a = primitive.vertices[primitive.indices[i]];
        glm::vec3 b = primitive.vertices[primitive.indices[i + 1]];
        glm::vec3 c = primitive.vertices[primitive.indices[i + 2]];

        // triangle plane normal and coefficient
        glm::vec3 triCross = glm::cross(b - a, c - a);
        if (triCross == glm::vec3(0))  // happens with some geometry idk
            continue;
        glm::vec3 planeNormal = glm::normalize(triCross);  // TODO avoid?
        float planeK = glm::dot(planeNormal, a);

        // intersect ray with plane
        float nDotD = glm::dot(planeNormal, dir);
        if (nDotD > -1e-6)
            continue;  // only front facing
        float t = (planeK - glm::dot(planeNormal, origin)) / nDotD;
        if (t <= 0 || t*t > *closestDist2)
            continue;
        glm::vec3 intersect = origin + dir * t;

        // double-area of smaller triangles defined by intersection point
        float dAreaQBC = glm::dot(glm::cross(c - b, intersect - b), planeNormal);
        float dAreaAQC = glm::dot(glm::cross(a - c, intersect - c), planeNormal);
        float dAreaABQ = glm::dot(glm::cross(b - a, intersect - a), planeNormal);

        // inside triangle?
        // check if inside all the edges
        if (dAreaQBC < 0 || dAreaAQC < 0 || dAreaABQ < 0)
            continue;  // not inside triangle

        closest = CollisionInfo();
        closest->point = intersect;
        closest->normal = planeNormal;
        *closestDist2 = t*t;
    }
    return closest;
}


}  // namespace
