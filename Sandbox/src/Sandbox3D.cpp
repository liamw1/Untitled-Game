#include "Sandbox3D.h"
#include <glm/gtc/matrix_transform.hpp>

Sandbox3D::Sandbox3D()
  : Layer("Sandbox3D"),
  m_CameraController(static_cast<radians>(glm::radians(45.0)), 1280.0f / 720.0f, static_cast<length_t>(0.1), static_cast<length_t>(100.0))
{
  Engine::RenderCommand::Initialize();
  Engine::Renderer::Initialize();
  m_CameraController.toggleFreeCam();
}

void Sandbox3D::onAttach()
{
  EN_PROFILE_FUNCTION();

  m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");
}

void Sandbox3D::onDetach()
{
}

void Sandbox3D::onUpdate(std::chrono::duration<seconds> timestep)
{
  EN_PROFILE_FUNCTION();

  // EN_TRACE("dt: {0}", timestep.count());

  m_CameraController.onUpdate(timestep);

  Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

  Engine::Renderer::BeginScene(m_CameraController.getCamera());
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      Engine::Renderer::DrawCube(Vec3((i - 1) * 1.25, 2.0, (j + 1) * 1.25), Vec3(1.0f), m_CheckerboardTexture);
  Engine::Renderer::EndScene();
}

void Sandbox3D::onImGuiRender()
{
}

void Sandbox3D::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}