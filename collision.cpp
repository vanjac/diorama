#include "collision.h"
#include <limits>
#include <glm/gtx/norm.hpp>

// :(

namespace diorama::physics {

static CollisionInfo raycastHierarchy(
    Component *component, glm::vec3 origin, glm::vec3 dir);
// dir must be a unit vector
static CollisionInfo raycastPrimitive(
    Component *component, const CollisionPrimitive *primitive,
    glm::vec3 origin, glm::vec3 dir, float *closestDist2);

static void sphereHierarchy(
    Component *component, Transform worldT, glm::vec3 center, float sqRadius,
    vector<CollisionInfo> &collisions);
static void spherePrimitive(
    Component *component, const CollisionPrimitive *primitive,
    Transform worldT, glm::vec3 center, float sqRadius,
    vector<CollisionInfo> &collisions);

CollisionInfo raycast(const World *world, glm::vec3 origin, glm::vec3 dir)
{
    CollisionInfo collision = raycastHierarchy(world->root(), origin, dir);
    if (collision.component)
        collision.normal = glm::normalize(collision.normal);
    return collision;
}

static CollisionInfo raycastHierarchy(
    Component *component, glm::vec3 origin, glm::vec3 dir)
{
    const Transform &t = component->tLocal();
    Transform invT = t.inverse();
    origin = invT.transformPoint(origin);
    dir = invT.transformVector(dir);  // TODO preserve length

    float closestDist2 = std::numeric_limits<float>::max();
    CollisionInfo closest;

    for (auto &child : component->children()) {
        auto collision = raycastHierarchy(child, origin, dir);
        if (collision.component) {
            float dist2 = glm::distance2(origin, collision.point);
            if (dist2 < closestDist2) {
                closestDist2 = dist2;
                closest = collision;
            }
        }
    }
    if (component->mesh && !component->mesh->collision.empty()) {
        // dir may not be a unit vector at this point
        dir = glm::normalize(dir);
        for (auto &primitive : component->mesh->collision) {
            auto collision = raycastPrimitive(
                component, &primitive, origin, dir, &closestDist2);
            if (collision.component) {
                closest = collision;
            }
        }
    }

    if (closest.component) {
        closest.point = t.transformPoint(closest.point);
        closest.normal = glm::transpose(glm::mat3(invT.matrix()))
            * closest.normal;
    }
    return closest;
}

static CollisionInfo raycastPrimitive(
    Component *component, const CollisionPrimitive *primitive,
    glm::vec3 origin, glm::vec3 dir, float *closestDist2)
{
    CollisionInfo closest;

    for (int i = 0; i < primitive->indices.size(); i += 3) {
        glm::vec3 a = primitive->vertices[primitive->indices[i]];
        glm::vec3 b = primitive->vertices[primitive->indices[i + 1]];
        glm::vec3 c = primitive->vertices[primitive->indices[i + 2]];

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

        closest.component = component;
        closest.point = intersect;
        closest.normal = planeNormal;
        *closestDist2 = t*t;
    }
    return closest;
}

void sphereCollision(const World *world, glm::vec3 center, float radius,
                     vector<CollisionInfo> &collisions)
{
    sphereHierarchy(world->root(), Transform(), center, radius*radius,
                    collisions);
}

static void sphereHierarchy(
    Component *component, Transform worldT, glm::vec3 center, float sqRadius,
    vector<CollisionInfo> &collisions)
{
    worldT *= component->tLocal();

    for (auto &child : component->children()){
        sphereHierarchy(child, worldT, center, sqRadius, collisions);
    }
    if (component->mesh && !component->mesh->collision.empty()) {
        for (auto &primitive : component->mesh->collision) {
            spherePrimitive(component, &primitive, worldT,
                            center, sqRadius, collisions);
        }
    }
}

static void spherePrimitive(
    Component *component, const CollisionPrimitive *primitive,
    Transform worldT, glm::vec3 center, float sqRadius,
    vector<CollisionInfo> &collisions)
{
    // https://gdbooks.gitbooks.io/3dcollisions/content/
    // some of this is copied from raycastPrimitive :(
    for (int i = 0; i < primitive->indices.size(); i += 3) {
        glm::vec3 a = primitive->vertices[primitive->indices[i]];
        glm::vec3 b = primitive->vertices[primitive->indices[i + 1]];
        glm::vec3 c = primitive->vertices[primitive->indices[i + 2]];
        a = worldT.transformPoint(a);
        b = worldT.transformPoint(b);
        c = worldT.transformPoint(c);

        glm::vec3 triCross = glm::cross(b - a, c - a);
        if (triCross == glm::vec3(0))  // happens with some geometry idk
            continue;
        glm::vec3 planeNorm = glm::normalize(triCross);  // TODO avoid?
        // planeK is also the distance to the origin (center of sphere)
        float planeK = glm::dot(planeNorm, a);

        float distToPlane = glm::dot(planeNorm, center) - planeK;
        glm::vec3 planePt = center - distToPlane * planeNorm;  // point on plane

        // check if inside each edge
        bool insideBC = glm::dot(glm::cross(c - b, planePt - b), planeNorm) >= 0;
        bool insideCA = glm::dot(glm::cross(a - c, planePt - c), planeNorm) >= 0;
        bool insideAB = glm::dot(glm::cross(b - a, planePt - a), planeNorm) >= 0;

        //  \  |            here's a triangle
        //   \ |
        //    \|            enjoy
        //     A
        //     |\
        //     | \
        //     |  \
        //     |   \
        // ----B----C----
        //     |     \
        //     |      \

        CollisionInfo collision;
        collision.normal = planeNorm;
        if (insideBC && insideCA && insideAB) {
            collision.point = planePt;
        } else {
            glm::vec3 e1, e2; // points forming an edge
            if (!insideBC) {
                e1 = b; e2 = c;
            } else if (!insideCA) {
                e1 = c; e2 = a;
            } else { // !insideAB
                e1 = a; e2 = b;
            }
            glm::vec3 edge = e2 - e1;
            float t = glm::dot(planePt - e1, edge) / glm::dot(edge, edge);
            t = glm::clamp(t, 0.0f, 1.0f);
            collision.point = e1 + t * edge;
        }

        if (glm::distance2(collision.point, center) <= sqRadius) {
            collisions.push_back(collision);
        }
    }
}

}  // namespace
