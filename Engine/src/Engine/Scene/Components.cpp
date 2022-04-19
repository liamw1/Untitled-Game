#include "ENpch.h"
#include "Components.h"
#include <glm/gtc/matrix_transform.hpp>

Vec3 Component::Transform::orientationDirection() const
{
  const length_t& roll = rotation.x;
  const length_t& pitch = rotation.y;
  const length_t& yaw = rotation.z;

  return { std::cos(yaw) * std::cos(pitch),
          -std::sin(yaw) * std::cos(pitch),
          -std::sin(pitch) };
}

Mat4 Component::Transform::calculateTransform() const
{
  Mat4 rotationMatrix = glm::rotate(Mat4(1.0), rotation.x, Vec3(1, 0, 0))
                      * glm::rotate(Mat4(1.0), rotation.y, Vec3(0, 1, 0))
                      * glm::rotate(Mat4(1.0), rotation.z, Vec3(0, 0, 1));

  return glm::translate(Mat4(1.0), position) * rotationMatrix * glm::scale(Mat4(1.0), scale);
}