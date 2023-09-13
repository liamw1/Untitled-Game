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

using BlockIndex2D = IVec2<blockIndex_t>;
using LocalIndex2D = IVec2<localIndex_t>;
using GlobalIndex2D = IVec2<globalIndex_t>;

using BlockIndex = IVec3<blockIndex_t>;
using LocalIndex = IVec3<localIndex_t>;
using GlobalIndex = IVec3<globalIndex_t>;

using BlockRect = IBox2<blockIndex_t>;
using LocalRect = IBox2<localIndex_t>;
using GlobalRect = IBox2<globalIndex_t>;

using BlockBox = IBox3<blockIndex_t>;
using LocalBox = IBox3<localIndex_t>;
using GlobalBox = IBox3<globalIndex_t>;

template<typename T> using BlockArrayRect = ArrayRect<T, blockIndex_t>;
template<typename T> using BlockArrayBox = ArrayBox<T, blockIndex_t>;
template<typename T> using ProtectedBlockArrayBox = Engine::Threads::ProtectedArrayBox<T, blockIndex_t>;