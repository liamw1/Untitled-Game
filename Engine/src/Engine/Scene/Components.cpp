#include "ENpch.h"
#include "Components.h"
#include <glm/gtc/matrix_transform.hpp>

namespace eng::component
{
  math::Vec3 Transform::orientationDirection() const
  {
    const length_t& roll = rotation.x;
    const length_t& pitch = rotation.y;
    const length_t& yaw = rotation.z;

    return {std::cos(yaw) * std::cos(pitch),
            -std::sin(yaw) * std::cos(pitch),
            -std::sin(pitch)};
  }

  math::Mat4 Transform::calculateTransform() const
  {
    math::Mat4 rotationMatrix = glm::rotate(math::Mat4(1.0), rotation.x, math::Vec3(1, 0, 0))
                              * glm::rotate(math::Mat4(1.0), rotation.y, math::Vec3(0, 1, 0))
                              * glm::rotate(math::Mat4(1.0), rotation.z, math::Vec3(0, 0, 1));

    return glm::translate(math::Mat4(1.0), position) * rotationMatrix * glm::scale(math::Mat4(1.0), scale);
  }
}