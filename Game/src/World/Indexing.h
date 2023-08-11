#pragma once
#include <Engine/Math/IBox.h>
#include <Engine/Utilities/BitUtilities.h>

// =========== Precision selection for Indices ============= //

template <bool isDoublePrecision> struct GlobalIndexSelector;

template<> struct GlobalIndexSelector<true> { using type = typename int64_t; };
template<> struct GlobalIndexSelector<false> { using type = typename int32_t; };

using blockIndex_t = int8_t;
using localIndex_t = int16_t;
using globalIndex_t = typename GlobalIndexSelector<std::is_same<double, length_t>::value>::type;

using SurfaceMapIndex = IVec2<globalIndex_t>;
using BlockIndex = IVec3<blockIndex_t>;
using LocalIndex = IVec3<localIndex_t>;
using GlobalIndex = IVec3<globalIndex_t>;

using BlockBox = IBox3<blockIndex_t>;



// ================= Hashes for indices =================== //

namespace std
{
  template<>
  struct hash<SurfaceMapIndex>
  {
    int operator()(const SurfaceMapIndex& index) const
    {
      return index.i % bitUi32(16) + bitUi32(16) * (index.j % bitUi32(16));
    }
  };

  template<>
  struct hash<GlobalIndex>
  {
    int operator()(const GlobalIndex& index) const
    {
      return index.i % bitUi32(10) + bitUi32(10) * (index.j % bitUi32(10)) + bitUi32(20) * (index.k % bitUi32(10));
    }
  };
}