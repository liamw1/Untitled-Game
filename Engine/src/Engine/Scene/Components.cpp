#include "ENpch.h"
#include "Components.h"
#include <glm/gtc/matrix_transform.hpp>

Vec3 Component::Transform::orientationDirection() const
{
  const auto& roll = rotation.x;
  const auto& pitch = rotation.y;
  const auto& yaw = rotation.z;

  return { cos(yaw) * cos(pitch),
          -sin(yaw) * cos(pitch),
          -sin(pitch) };
}

Mat4 Component::Transform::calculateTransform() const
{
  Mat4 rotationMatrix = glm::rotate(Mat4(1.0), rotation.x, Vec3(1, 0, 0))
                      * glm::rotate(Mat4(1.0), rotation.y, Vec3(0, 1, 0))
                      * glm::rotate(Mat4(1.0), rotation.z, Vec3(0, 0, 1));

  return glm::translate(Mat4(1.0), position) * rotationMatrix * glm::scale(Mat4(1.0), scale);
}
