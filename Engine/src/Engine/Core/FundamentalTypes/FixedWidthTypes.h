#pragma once
#include "Int.h"
#include "Float.h"

// Unsigned integer types
using u8 = eng::Int<std::uint8_t>;
using u16 = eng::Int<std::uint16_t>;
using u32 = eng::Int<std::uint32_t>;
using u64 = eng::Int<std::uint64_t>;
using uSize = eng::Int<std::size_t>;

// Signed integer types
using i8 = eng::Int<std::int8_t>;
using i16 = eng::Int<std::int16_t>;
using i32 = eng::Int<std::int32_t>;
using i64 = eng::Int<std::int64_t>;
using iSize = eng::Int<std::ptrdiff_t>;

// Floating point types    C++23: Can use standard fixed-width floating point types
using f32 = eng::Float<float>;
using f64 = eng::Float<double>;
