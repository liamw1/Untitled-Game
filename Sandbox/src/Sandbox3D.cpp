#include "SBpch.h"
#include "Sandbox3D.h"

Sandbox3D::Sandbox3D()
  : Layer("Sandbox3D")
{
  Engine::RenderCommand::Initialize();
  Engine::Renderer::Initialize();
}

void Sandbox3D::onAttach()
{
  EN_PROFILE_FUNCTION();

  m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");

  m_CameraEntity = Engine::Scene::CreateEntity();
  m_CameraEntity.add<Component::Transform>();
  m_CameraEntity.add<Component::Camera>().isActive = true;
  m_CameraEntity.get<Component::Camera>().camera.setPerspective(1.0f, Angle(90.0f), 0.2f, 100.0f);
}

void Sandbox3D::onDetach()
{
}

void Sandbox3D::onUpdate(Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  Engine::RenderCommand::Clear(Float4(0.1f, 0.1f, 0.1f, 1.0f));

  Engine::Renderer::BeginScene(Engine::Scene::ActiveCameraViewProjection());
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      Engine::Renderer::DrawCube(Vec3(static_cast<length_t>((i - 1) * 1.25), 2.0, static_cast<length_t>((j + 1) * 1.25)), Vec3(1.0f), m_CheckerboardTexture);
  Engine::Renderer::EndScene();
}

void Sandbox3D::onImGuiRender()
{
}

void Sandbox3D::onEvent(Engine::Event& event)
{
}