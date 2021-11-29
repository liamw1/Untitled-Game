#include "GMpch.h"
#include "GameSandbox.h"
#include "World/ChunkRenderer.h"
#include "World/World.h"

GameSandbox::GameSandbox()
  : Layer("GameSandbox"),
    m_Player(),
    m_World(glm::vec3(0.0f))
{
  Engine::RenderCommand::Initialize();
  ChunkRenderer::Initialize();

  m_Player.setPosition({ Chunk::Length() / 2, Chunk::Length() / 2, 2 * Chunk::Length() });
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

  // EN_TRACE("fps: {0}", static_cast<int>(1.0f / timestep.count()));

#if 0
  EN_TRACE("({0}, {1}, {2})", static_cast<int>(m_Player.getPosition()[0] / Block::Length()),
                              static_cast<int>(m_Player.getPosition()[1] / Block::Length()),
                              static_cast<int>(m_Player.getPosition()[2] / Block::Length()));
#endif

#if 0
  ChunkIndex chunk = Chunk::ChunkIndexFromPos(m_Player.getPosition());
  EN_TRACE("({0}, {1}, {2})", chunk.i, chunk.j, chunk.k);
#endif

  Engine::RenderCommand::Clear({ 0.788f, 0.949f, 0.949f, 1.0f });

  m_World.onUpdate(timestep, m_Player);
}

void GameSandbox::onImGuiRender()
{
}

void GameSandbox::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));

  m_Player.onEvent(event);
  m_World.onEvent(event);
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
