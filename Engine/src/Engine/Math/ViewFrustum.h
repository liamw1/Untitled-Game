#pragma once
#include "Vec.h"
#include "Engine/Utilities/EnumUtilities.h"

namespace eng::math
{
  enum class FrustumPlane
  {
    Left, Right, Bottom, Top, Near, Far,

    First = 0, Last = Far
  };

  /*
  Uses algorithm described in
  https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

  \returns An array of vectors representing the view frustum planes.
           For a plane of the form Ax + By + Cz + D = 0,
           its corresponding vector is {A, B, C, D}.
  */
  EnumArray<Vec4, FrustumPlane> calculateViewFrustumPlanes(const eng::math::Mat4& viewProjection);

  /*
    \returns True if the given point is inside the given set of frustum planes.
             Could be any frustum, not necessarily the view frustum.
  */
  bool isInFrustum(const eng::math::Vec3& point, const EnumArray<Vec4, FrustumPlane>& frustumPlanes);
}