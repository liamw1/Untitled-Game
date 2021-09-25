#include "GameSandbox.h"
#include <World/ChunkRenderer.h>

GameSandbox::GameSandbox()
  : Layer("GameSandbox"),
    m_CameraController(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f),
  m_Chunk({ 0.0f, 0.0f, -10.0f }, Block::Sand)
{
  Engine::RenderCommand::Initialize();
  ChunkRenderer::Initialize();
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

  EN_TRACE("dt: {0}", timestep.count());

  m_CameraController.onUpdate(timestep);

  Engine::RenderCommand::Clear({ 0.788f, 0.949f, 0.949f, 1.0f });

  ChunkRenderer::BeginScene(m_CameraController.getCamera());
  m_Chunk.render();
  ChunkRenderer::EndScene();
}

void GameSandbox::onImGuiRender()
{
}

void GameSandbox::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));

  m_CameraController.onEvent(event);
}

bool GameSandbox::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  static bool wireFrameMode = false;

  if (event.getKeyCode() == Key::F1)
  {
    wireFrameMode = !wireFrameMode;
    Engine::RenderCommand::WireFrameToggle(wireFrameMode);
  }

  return false;
}
