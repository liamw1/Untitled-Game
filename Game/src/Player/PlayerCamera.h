#pragma once
#include "Block/Block.h"

class CameraController : public eng::EntityScript
{
  // Camera properties
  static constexpr f32 c_CameraSensitivity = 0.1f;
  static constexpr f32 c_CameraZoomSensitivity = 0.2f;

  static constexpr eng::math::Angle c_MinPitch = -89.99_deg;
  static constexpr eng::math::Angle c_MaxPitch = 89.99_deg;
  static constexpr eng::math::Angle c_MinFOV = 0.5_deg;
  static constexpr eng::math::Angle c_MaxFOV = 80_deg;

  static constexpr eng::math::Vec3 c_UpDirection = eng::math::Vec3(0, 0, 1);

  // Camera initialization
  static constexpr eng::math::Angle c_FOV = 80_deg;
  static constexpr f32 c_AspectRatio = 1280.0f / 720.0f;
  static constexpr length_t c_NearClip = 0.125_m * block::length();

  eng::Entity m_Entity;
  eng::math::Float2 m_LastMousePosition;

public:
  CameraController(eng::Entity entity);

  void onUpdate(eng::Timestep timestep) override;

  void onEvent(eng::event::Event& event) override;

private:
  bool onMouseMove(eng::event::MouseMove& event);
  bool onMouseScroll(eng::event::MouseScroll& event);
};