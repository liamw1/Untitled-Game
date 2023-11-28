#include "ENpch.h"
#include "ViewFrustum.h"

#include <glm/gtc/matrix_access.hpp>

namespace eng::math
{
  EnumArray<Vec4, FrustumPlane> calculateViewFrustumPlanes(const eng::math::Mat4& viewProjection)
  {
    eng::math::Vec4 row1 = glm::row(viewProjection, 0);
    eng::math::Vec4 row2 = glm::row(viewProjection, 1);
    eng::math::Vec4 row3 = glm::row(viewProjection, 2);
    eng::math::Vec4 row4 = glm::row(viewProjection, 3);

    return { { FrustumPlane::Left,    row4 + row1 },
             { FrustumPlane::Right,   row4 - row1 },
             { FrustumPlane::Bottom,  row4 + row2 },
             { FrustumPlane::Top,     row4 - row2 },
             { FrustumPlane::Near,    row4 + row3 } };
  }

  bool isInFrustum(const eng::math::Vec3& point, const EnumArray<Vec4, FrustumPlane>& frustumPlanes)
  {
    return eng::algo::noneOf(frustumPlanes, [&point](const Vec4& plane) { return glm::dot(eng::math::Vec4(point, 1), plane) < 0; });
  }
}
