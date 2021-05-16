#pragma once

#include <glm/glm.hpp>

class Transform
{
public:
    static const glm::vec3 RIGHT;
    static const glm::vec3 LEFT;
    static const glm::vec3 FORWARD;
    static const glm::vec3 BACK;
    static const glm::vec3 UP;
    static const glm::vec3 DOWN;

    Transform();
    Transform(glm::mat4 matrix);

    static Transform translate(const glm::vec3 &v);
    static Transform rotate(float angle, const glm::vec3 &axis);
    static Transform scale(const glm::vec3 &v);

    const glm::mat4 & matrix() const;
    
    const Transform operator*(const Transform &rhs) const;
    Transform & operator*=(const Transform &rhs);
    Transform inverse() const;

    glm::vec3 position() const;
    // these are NOT normalized
    glm::vec3 right() const;
    glm::vec3 left() const;
    glm::vec3 forward() const;
    glm::vec3 back() const;
    glm::vec3 up() const;
    glm::vec3 down() const;

    glm::vec3 transformVector(glm::vec3 v) const;
    glm::vec3 transformPoint(glm::vec3 p) const;

private:
    glm::mat4 mat;
};
