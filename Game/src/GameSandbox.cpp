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
  EN_PROFILE_FUNCTION();
}

void GameSandbox::onDetach()
{
  EN_PROFILE_FUNCTION();
}

void GameSandbox::onUpdate(std::chrono::duration<float> timestep)
{
  EN_PROFILE_FUNCTION();

  EN_TRACE("dt: {0}", timestep.count());

  m_CameraController.onUpdate(timestep);

  Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

  ChunkRenderer::BeginScene(m_CameraController.getCamera());
  m_Chunk.render();
  ChunkRenderer::EndScene();
}

void GameSandbox::onImGuiRender()
{
}

void GameSandbox::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}
