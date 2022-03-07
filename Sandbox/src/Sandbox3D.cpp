#include "SBpch.h"
#include "Sandbox3D.h"

Sandbox3D::Sandbox3D()
  : Layer("Sandbox3D"),
  m_CameraController(Engine::Angle(45.0f), 1280.0f / 720.0f, static_cast<length_t>(0.1), static_cast<length_t>(100.0))
{
  Engine::RenderCommand::Initialize();
  Engine::Renderer::Initialize();
  m_CameraController.toggleFreeCam();
}

void Sandbox3D::onAttach()
{
  EN_PROFILE_FUNCTION();

  m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");

  m_CameraEntity = Engine::Scene::CreateEntity();
  m_CameraEntity.add<Engine::Component::Camera>().isActive = true;
}

void Sandbox3D::onDetach()
{
}

void Sandbox3D::onUpdate(Engine::Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  m_CameraController.onUpdate(timestep);
  m_CameraEntity.get<Engine::Component::Camera>().camera.setProjection(m_CameraController.getViewProjectionMatrix());

  Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

  Engine::Renderer::BeginScene(Engine::Scene::GetActiveCamera().getProjection());
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