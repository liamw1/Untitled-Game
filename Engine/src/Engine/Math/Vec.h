#pragma once
#include "Engine/Core/Units.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace eng::math
{
  using Vec2 = glm::vec<2, length_t, glm::defaultp>;
  using Vec3 = glm::vec<3, length_t, glm::defaultp>;
  using Vec4 = glm::vec<4, length_t, glm::defaultp>;
  using Mat3 = glm::mat<3, 3, length_t, glm::defaultp>;
  using Mat4 = glm::mat<4, 4, length_t, glm::defaultp>;
  using Quat = glm::qua<length_t, glm::defaultp>;

  using Float2 = glm::vec2;
  using Float3 = glm::vec3;
  using Float4 = glm::vec4;
  using FMat3 = glm::mat3;
  using FMat4 = glm::mat4;
  using FQuat = glm::quat;
  using Double2 = glm::dvec2;
  using Double3 = glm::dvec3;
  using Double4 = glm::dvec4;
  using DMat3 = glm::dmat3;
  using DMat4 = glm::dmat4;
  using DQuat = glm::dquat;
}

// Missing operators for glm vectors
constexpr glm::vec2 operator*(f64 x, glm::vec2 v) { return v *= eng::arithmeticCastUnchecked<f32>(x); }
constexpr glm::vec2 operator*(glm::vec2 v, f64 x) { return v *= eng::arithmeticCastUnchecked<f32>(x); }
constexpr glm::vec2 operator/(glm::vec2 v, f64 x) { return v /= eng::arithmeticCastUnchecked<f32>(x); }

constexpr glm::vec3 operator*(f64 x, glm::vec3 v) { return v *= eng::arithmeticCastUnchecked<f32>(x); }
constexpr glm::vec3 operator*(glm::vec3 v, f64 x) { return v *= eng::arithmeticCastUnchecked<f32>(x); }
constexpr glm::vec3 operator/(glm::vec3 v, f64 x) { return v /= eng::arithmeticCastUnchecked<f32>(x); }

constexpr glm::vec4 operator*(f64 x, glm::vec4 v) { return v *= eng::arithmeticCastUnchecked<f32>(x); }
constexpr glm::vec4 operator*(glm::vec4 v, f64 x) { return v *= eng::arithmeticCastUnchecked<f32>(x); }
constexpr glm::vec4 operator/(glm::vec4 v, f64 x) { return v /= eng::arithmeticCastUnchecked<f32>(x); }

constexpr glm::dvec2 operator*(f32 x, glm::dvec2 v) { return v *= eng::arithmeticUpcast<f64>(x); }
constexpr glm::dvec2 operator*(glm::dvec2 v, f32 x) { return v *= eng::arithmeticUpcast<f64>(x); }
constexpr glm::dvec2 operator/(glm::dvec2 v, f32 x) { return v /= eng::arithmeticUpcast<f64>(x); }

constexpr glm::dvec3 operator*(f32 x, glm::dvec3 v) { return v *= eng::arithmeticUpcast<f64>(x); }
constexpr glm::dvec3 operator*(glm::dvec3 v, f32 x) { return v *= eng::arithmeticUpcast<f64>(x); }
constexpr glm::dvec3 operator/(glm::dvec3 v, f32 x) { return v /= eng::arithmeticUpcast<f64>(x); }

constexpr glm::dvec4 operator*(f32 x, glm::dvec4 v) { return v *= eng::arithmeticUpcast<f64>(x); }
constexpr glm::dvec4 operator*(glm::dvec4 v, f32 x) { return v *= eng::arithmeticUpcast<f64>(x); }
constexpr glm::dvec4 operator/(glm::dvec4 v, f32 x) { return v /= eng::arithmeticUpcast<f64>(x); }

// Operator overloads for ostream
namespace std
{
  template<typename T, glm::qualifier Q>
  inline ostream& operator<<(ostream& os, const glm::vec<2, T, Q>& v) { return os << '(' << v.x << ", " << v.y << ')'; }

  template<typename T, glm::qualifier Q>
  inline ostream& operator<<(ostream& os, const glm::vec<3, T, Q>& v) { return os << '(' << v.x << ", " << v.y << ", " << v.z << ')'; }

  template<typename T, glm::qualifier Q>
  inline ostream& operator<<(ostream& os, const glm::vec<4, T, Q>& v) { return os << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')'; }

  template<typename T, glm::qualifier Q>
  inline ostream& operator<<(ostream& os, const glm::mat<3, 3, T, Q>& m) { return os << m[0] << '\n' << m[1] << '\n' << m[2]; }

  template<typename T, glm::qualifier Q>
  inline ostream& operator<<(ostream& os, const glm::mat<4, 4, T, Q>& m) { return os << m[0] << '\n' << m[1] << '\n' << m[2] << '\n' << m[3]; }
}