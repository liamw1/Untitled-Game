#include "GMpch.h"
#include "GameSandbox.h"
#include "World/ChunkRenderer.h"
#include "World/World.h"

GameSandbox::GameSandbox()
  : Layer("GameSandbox"),
    m_CameraController(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f)
{
  Engine::RenderCommand::Initialize();
  ChunkRenderer::Initialize();

  m_CameraController.setPosition({ Chunk::Length() / 2, Chunk::Length() / 2, 2 * Chunk::Length() });
  World::Initialize(m_CameraController.getCamera().getPosition());
}

void GameSandbox::onAttach()
{
}

void GameSandbox::onDetach()
{
}

void GameSandbox::onUpdate(std::chrono::duration<float> timestep)
{
  EN_PROFILE_FUNCTION();

  EN_TRACE("fps: {0}", static_cast<int>(1.0f / timestep.count()));

  m_CameraController.onUpdate(timestep);

  Engine::RenderCommand::Clear({ 0.788f, 0.949f, 0.949f, 1.0f });

  World::OnUpdate(m_CameraController.getCamera());
}

void GameSandbox::onImGuiRender()
{
}

void GameSandbox::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));

  m_CameraController.onEvent(event);
  World::OnEvent(event);
}

bool GameSandbox::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  static bool wireFrameEnabled = false;
  static bool faceCullingEnabled = false;

  if (event.getKeyCode() == Key::F1)
  {
    wireFrameEnabled = !wireFrameEnabled;
    Engine::RenderCommand::WireFrameToggle(wireFrameEnabled);
  }
  if (event.getKeyCode() == Key::F2)
  {
    faceCullingEnabled = !faceCullingEnabled;
    Engine::RenderCommand::FaceCullToggle(faceCullingEnabled);
  }

  return false;
}
