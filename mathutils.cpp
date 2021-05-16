#include "mathutils.h"
#include <glm/ext/matrix_transform.hpp>

// blender coordinate system
// (the only good coordinate system)
const glm::vec3 Transform::RIGHT    (1, 0, 0);
const glm::vec3 Transform::LEFT     (-1, 0, 0);
const glm::vec3 Transform::FORWARD  (0, 1, 0);
const glm::vec3 Transform::BACK     (0, -1, 0);
const glm::vec3 Transform::UP       (0, 0, 1);
const glm::vec3 Transform::DOWN     (0, 0, -1);

Transform::Transform()
    : mat(1)
{}

Transform::Transform(glm::mat4 matrix)
    : mat(matrix)
{}

Transform Transform::translate(const glm::vec3 &v)
{
    return glm::translate(glm::mat4(1), v);
}

Transform Transform::rotate(float angle, const glm::vec3 &axis)
{
    return glm::rotate(glm::mat4(1), angle, axis);
}

Transform Transform::scale(const glm::vec3 &v)
{
    return glm::scale(glm::mat4(1), v);
}

const glm::mat4 & Transform::matrix() const
{
    return mat;
}

const Transform Transform::operator*(const Transform &rhs) const
{
    return this->mat * rhs.mat;
}

Transform & Transform::operator*=(const Transform &rhs)
{
    mat *= rhs.mat;
    return *this;
}

Transform Transform::inverse() const
{
    return glm::inverse(mat);
}

glm::vec3 Transform::position() const
{
    return glm::vec3(mat[3]);
}

glm::vec3 Transform::right() const
{
    return glm::vec3(mat[0]);
}

glm::vec3 Transform::left() const
{
    return -glm::vec3(mat[0]);
}

glm::vec3 Transform::forward() const
{
    return glm::vec3(mat[1]);
}

glm::vec3 Transform::back() const
{
    return -glm::vec3(mat[1]);
}

glm::vec3 Transform::up() const
{
    return glm::vec3(mat[2]);
}

glm::vec3 Transform::down() const
{
    return -glm::vec3(mat[2]);
}

glm::vec3 Transform::transformVector(glm::vec3 v) const
{
    return mat * glm::vec4(v, 0);
}

glm::vec3 Transform::transformPoint(glm::vec3 v) const
{
    return mat * glm::vec4(v, 1);
}
