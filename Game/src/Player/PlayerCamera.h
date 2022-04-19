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
  static constexpr float s_CameraSensitivity = 0.1f;
  static constexpr float s_CameraZoomSensitivity = 0.2f;
  static constexpr length_t s_TranslationSpeed = 320 * Block::Length();

  static constexpr Angle s_MinPitch = Angle(-89.99f);
  static constexpr Angle s_MaxPitch = Angle(89.99f);
  static constexpr Angle s_MinFOV = Angle(0.5f);
  static constexpr Angle s_MaxFOV = Angle(80.0f);

  static constexpr Vec3 s_UpDirection = Vec3(0, 0, 1);

  // Camera initialization
  static constexpr Angle s_FOV = Angle(80.0f);
  static constexpr float s_AspectRatio = 1280.0f / 720.0f;
  static constexpr length_t s_NearClip = static_cast<length_t>(0.125 * Block::Length());
  static constexpr length_t s_FarClip = static_cast<length_t>(10000 * Block::Length());

  Engine::Entity m_Entity;
  Float2 m_LastMousePosition{};

  bool onMouseMove(Engine::MouseMoveEvent& event);
  bool onMouseScroll(Engine::MouseScrollEvent& event);
};