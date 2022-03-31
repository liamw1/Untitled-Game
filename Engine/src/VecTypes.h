#pragma once
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

// =========== Precision selection for vectors/matrices ============= //

template <bool isDoublePrecision> struct Vec2Selector;
template <bool isDoublePrecision> struct Vec3Selector;
template <bool isDoublePrecision> struct Vec4Selector;
template <bool isDoublePrecision> struct Mat3Selector;
template <bool isDoublePrecision> struct Mat4Selector;
template <bool isDoublePrecision> struct QuatSelector;

template<> struct Vec2Selector<true> { using type = typename glm::dvec2; };
template<> struct Vec3Selector<true> { using type = typename glm::dvec3; };
template<> struct Vec4Selector<true> { using type = typename glm::dvec4; };
template<> struct Mat3Selector<true> { using type = typename glm::dmat3; };
template<> struct Mat4Selector<true> { using type = typename glm::dmat4; };
template<> struct QuatSelector<true> { using type = typename glm::dquat; };

template<> struct Vec2Selector<false> { using type = typename glm::vec2; };
template<> struct Vec3Selector<false> { using type = typename glm::vec3; };
template<> struct Vec4Selector<false> { using type = typename glm::vec4; };
template<> struct Mat3Selector<false> { using type = typename glm::mat3; };
template<> struct Mat4Selector<false> { using type = typename glm::mat4; };
template<> struct QuatSelector<false> { using type = typename glm::quat; };

using Vec2 = typename Vec2Selector<std::is_same<double, length_t>::value>::type;
using Vec3 = typename Vec3Selector<std::is_same<double, length_t>::value>::type;
using Vec4 = typename Vec4Selector<std::is_same<double, length_t>::value>::type;
using Mat3 = typename Mat3Selector<std::is_same<double, length_t>::value>::type;
using Mat4 = typename Mat4Selector<std::is_same<double, length_t>::value>::type;
using Quat = typename QuatSelector<std::is_same<double, length_t>::value>::type;

using Float2 = glm::vec2;
using Float3 = glm::vec3;
using Float4 = glm::vec4;
using Float3x3 = glm::mat3;
using Float4x4 = glm::mat4;
using FQuat = glm::quat;
using Double2 = glm::dvec2;
using Double3 = glm::dvec3;
using Double4 = glm::dvec4;
using Double3x3 = glm::dmat3;
using Double4x4 = glm::dmat4;
using DQuat = glm::dquat;

// Missing operators for glm vectors
constexpr glm::vec2 operator*(double x, glm::vec2 v) { return v *= static_cast<float>(x); }
constexpr glm::vec2 operator*(glm::vec2 v, double x) { return v *= static_cast<float>(x); }
constexpr glm::vec2 operator/(glm::vec2 v, double x) { return v /= static_cast<float>(x); }

constexpr glm::vec3 operator*(double x, glm::vec3 v) { return v *= static_cast<float>(x); }
constexpr glm::vec3 operator*(glm::vec3 v, double x) { return v *= static_cast<float>(x); }
constexpr glm::vec3 operator/(glm::vec3 v, double x) { return v /= static_cast<float>(x); }

constexpr glm::vec4 operator*(double x, glm::vec4 v) { return v *= static_cast<float>(x); }
constexpr glm::vec4 operator*(glm::vec4 v, double x) { return v *= static_cast<float>(x); }
constexpr glm::vec4 operator/(glm::vec4 v, double x) { return v /= static_cast<float>(x); }

constexpr glm::dvec2 operator*(float x, glm::dvec2 v) { return v *= static_cast<double>(x); }
constexpr glm::dvec2 operator*(glm::dvec2 v, float x) { return v *= static_cast<double>(x); }
constexpr glm::dvec2 operator/(glm::dvec2 v, float x) { return v /= static_cast<double>(x); }

constexpr glm::dvec3 operator*(float x, glm::dvec3 v) { return v *= static_cast<double>(x); }
constexpr glm::dvec3 operator*(glm::dvec3 v, float x) { return v *= static_cast<double>(x); }
constexpr glm::dvec3 operator/(glm::dvec3 v, float x) { return v /= static_cast<double>(x); }

constexpr glm::dvec4 operator*(float x, glm::dvec4 v) { return v *= static_cast<double>(x); }
constexpr glm::dvec4 operator*(glm::dvec4 v, float x) { return v *= static_cast<double>(x); }
constexpr glm::dvec4 operator/(glm::dvec4 v, float x) { return v /= static_cast<double>(x); }

// Operator overloads for ostream
namespace std
{
  inline ostream& operator<<(ostream& os, const glm::vec2& v) { return os << '(' << v.x << ", " << v.y << ')'; }
  inline ostream& operator<<(ostream& os, const glm::vec3& v) { return os << '(' << v.x << ", " << v.y << ", " << v.z << ')'; }
  inline ostream& operator<<(ostream& os, const glm::vec4& v) { return os << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')'; }

  inline ostream& operator<<(ostream& os, const glm::dvec2& v) { return os << '(' << v.x << ", " << v.y << ')'; }
  inline ostream& operator<<(ostream& os, const glm::dvec3& v) { return os << '(' << v.x << ", " << v.y << ", " << v.z << ')'; }
  inline ostream& operator<<(ostream& os, const glm::dvec4& v) { return os << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')'; }

  inline ostream& operator<<(ostream& os, const glm::mat3& m) { return os << m[0] << '\n' << m[1] << '\n' << m[2]; }
  inline ostream& operator<<(ostream& os, const glm::mat4& m) { return os << m[0] << '\n' << m[1] << '\n' << m[2] << '\n' << m[3]; }

  inline ostream& operator<<(ostream& os, const glm::dmat3& m) { return os << m[0] << '\n' << m[1] << '\n' << m[2]; }
  inline ostream& operator<<(ostream& os, const glm::dmat4& m) { return os << m[0] << '\n' << m[1] << '\n' << m[2] << '\n' << m[3]; }
}