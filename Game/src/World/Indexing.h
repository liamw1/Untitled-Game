#pragma once
#include <Engine/Math/ArrayRect.h>
#include <Engine/Threads/Containers/ProtectedArrayBox.h>

// =========== Precision selection for Indices ============= //
template <bool isDoublePrecision> struct GlobalIndexSelector;

template<> struct GlobalIndexSelector<true> { using type = typename int64_t; };
template<> struct GlobalIndexSelector<false> { using type = typename int32_t; };

using blockIndex_t = int8_t;
using localIndex_t = int16_t;
using globalIndex_t = typename GlobalIndexSelector<std::is_same<double, length_t>::value>::type;

using BlockIndex2D = eng::math::IVec2<blockIndex_t>;
using LocalIndex2D = eng::math::IVec2<localIndex_t>;
using GlobalIndex2D = eng::math::IVec2<globalIndex_t>;

using BlockIndex = eng::math::IVec3<blockIndex_t>;
using LocalIndex = eng::math::IVec3<localIndex_t>;
using GlobalIndex = eng::math::IVec3<globalIndex_t>;

using BlockRect = eng::math::IBox2<blockIndex_t>;
using LocalRect = eng::math::IBox2<localIndex_t>;
using GlobalRect = eng::math::IBox2<globalIndex_t>;

using BlockBox = eng::math::IBox3<blockIndex_t>;
using LocalBox = eng::math::IBox3<localIndex_t>;
using GlobalBox = eng::math::IBox3<globalIndex_t>;

template<typename T> using BlockArrayRect = eng::math::ArrayRect<T, blockIndex_t>;
template<typename T> using BlockArrayBox = eng::math::ArrayBox<T, blockIndex_t>;
template<typename T> using ProtectedBlockArrayBox = eng::threads::ProtectedArrayBox<T, blockIndex_t>;