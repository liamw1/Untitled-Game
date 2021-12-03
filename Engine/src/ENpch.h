#pragma once

// ==================== Standard Libraries ==================== //
#include <cmath>
#include <chrono>
#include <memory>

// Data structures
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <functional>
#include <map>
#include <unordered_map>

// I/O
#include <iostream>
#include <fstream>
#include <filesystem>

// ==================== Common Utilities ==================== //
#include "Engine/Core/Core.h"
#include "Engine/Core/Log.h"
#include "Engine/Debug/Instrumentor.h"

#ifdef EN_PLATFORM_WINDOWS
  #include <Windows.h>
#endif

// =========================== Math ========================= //
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>


// =========== Precision selection for vectors/matrices ============= //

template <bool isDoublePrecision> struct vec2Selector;
template <bool isDoublePrecision> struct vec3Selector;
template <bool isDoublePrecision> struct vec4Selector;
template <bool isDoublePrecision> struct mat3Selector;
template <bool isDoublePrecision> struct mat4Selector;

template<> struct vec2Selector<true> { using type = typename glm::dvec2; };
template<> struct vec3Selector<true> { using type = typename glm::dvec3; };
template<> struct vec4Selector<true> { using type = typename glm::dvec4; };
template<> struct mat3Selector<true> { using type = typename glm::dmat3; };
template<> struct mat4Selector<true> { using type = typename glm::dmat4; };

template<> struct vec2Selector<false> { using type = typename glm::vec2; };
template<> struct vec3Selector<false> { using type = typename glm::vec3; };
template<> struct vec4Selector<false> { using type = typename glm::vec4; };
template<> struct mat3Selector<false> { using type = typename glm::mat3; };
template<> struct mat4Selector<false> { using type = typename glm::mat4; };

using Vec2 = typename vec2Selector<std::is_same<double, length_t>::value>::type;
using Vec3 = typename vec3Selector<std::is_same<double, length_t>::value>::type;
using Vec4 = typename vec4Selector<std::is_same<double, length_t>::value>::type;
using Mat3 = typename mat3Selector<std::is_same<double, length_t>::value>::type;
using Mat4 = typename mat4Selector<std::is_same<double, length_t>::value>::type;

using Float2 = glm::vec2;  using Double2 = glm::dvec2;
using Float3 = glm::vec3;  using Double3 = glm::dvec3;
using Float4 = glm::vec4;  using Double4 = glm::dvec4;
using Float3x3 = glm::mat3;  using Double3x3 = glm::dmat3;
using Float4x4 = glm::mat4;  using Double4x4 = glm::dmat4;

// Missing operators for glm vectors
constexpr glm::vec2 operator*(double x, glm::vec2 vec) { return vec *= x; }
constexpr glm::vec2 operator*(glm::vec2 vec, double x) { return vec *= x; }
constexpr glm::vec2 operator/(glm::vec2 vec, double x) { return vec /= x; }

constexpr glm::vec3 operator*(double x, glm::vec3 vec) { return vec *= x; }
constexpr glm::vec3 operator*(glm::vec3 vec, double x) { return vec *= x; }
constexpr glm::vec3 operator/(glm::vec3 vec, double x) { return vec /= x; }

constexpr glm::dvec2 operator*(float x, glm::dvec2 vec) { return vec *= x; }
constexpr glm::dvec2 operator*(glm::dvec2 vec, float x) { return vec *= x; }
constexpr glm::dvec2 operator/(glm::dvec2 vec, float x) { return vec /= x; }

constexpr glm::dvec3 operator*(float x, glm::dvec3 vec) { return vec *= x; }
constexpr glm::dvec3 operator*(glm::dvec3 vec, float x) { return vec *= x; }
constexpr glm::dvec3 operator/(glm::dvec3 vec, float x) { return vec /= x; }