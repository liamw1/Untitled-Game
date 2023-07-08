#pragma once
#include "Block/Block.h"
#include <Engine.h>

class CameraController : public Engine::EntityScript
{
public:
  CameraController(Engine::Entity entity);

  void onUpdate(Timestep timestep) override;
  void onEvent(Engine::Event& event) override;

private:
  // Camera properties
  static constexpr float c_CameraSensitivity = 0.1f;
  static constexpr float c_CameraZoomSensitivity = 0.2f;
  static constexpr length_t c_TranslationSpeed = 50 * Block::Length();

  static constexpr Angle c_MinPitch = -89.99_deg;
  static constexpr Angle c_MaxPitch = 89.99_deg;
  static constexpr Angle c_MinFOV = 0.5_deg;
  static constexpr Angle c_MaxFOV = 80_deg;

  static constexpr Vec3 c_UpDirection = Vec3(0, 0, 1);

  // Camera initialization
  static constexpr Angle c_FOV = 80_deg;
  static constexpr float c_AspectRatio = 1280.0f / 720.0f;
  static constexpr length_t c_NearClip = 0.125_m * Block::Length();
  static constexpr length_t c_FarClip = 10000_m * Block::Length();

  Engine::Entity m_Entity;
  Float2 m_LastMousePosition{};

  bool onMouseMove(Engine::MouseMoveEvent& event);
  bool onMouseScroll(Engine::MouseScrollEvent& event);
};