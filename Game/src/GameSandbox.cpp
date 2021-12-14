#include "GMpch.h"
#include "GameSandbox.h"
#include "World/ChunkRenderer.h"
#include "World/World.h"
#include "World/LOD.h"

GameSandbox::GameSandbox()
  : Layer("GameSandbox")
{
  Block::Initialize();
  Player::Initialize({ 0, 0, 1 }, Block::Length() * Vec3(16.0));
  Engine::RenderCommand::Initialize();
  Engine::Renderer::Initialize();
  ChunkRenderer::Initialize();
  m_World.initialize();
}

void GameSandbox::onAttach()
{
}

void GameSandbox::onDetach()
{
}

void GameSandbox::onUpdate(std::chrono::duration<seconds> timestep)
{
  EN_PROFILE_FUNCTION();

  // EN_TRACE("fps: {0}", static_cast<int>(1.0f / timestep.count()));

#if 0
  EN_TRACE("({0}, {1}, {2})", static_cast<int>(m_Player.getPosition()[0] / Block::Length()),
                              static_cast<int>(m_Player.getPosition()[1] / Block::Length()),
                              static_cast<int>(m_Player.getPosition()[2] / Block::Length()));
#endif

#if 0
  LocalIndex chunk = Chunk::ChunkIndexFromPos(m_Player.getPosition());
  EN_TRACE("({0}, {1}, {2})", chunk.i, chunk.j, chunk.k);
#endif

  Engine::RenderCommand::Clear({ 0.788f, 0.949f, 0.949f, 1.0f });

  m_World.onUpdate(timestep);
}

void GameSandbox::onImGuiRender()
{
}

void GameSandbox::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));

  Player::OnEvent(event);
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
