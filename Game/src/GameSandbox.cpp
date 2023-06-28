#include "GMpch.h"
#include "GameSandbox.h"
#include "Player/Player.h"
#include "World/World.h"
#include "World/Biome.h"
#include "World/LOD.h"

GameSandbox::GameSandbox()
  : Layer("GameSandbox")
{
  Player::Initialize(GlobalIndex(0, 0, 2), Block::Length() * Vec3(16.0));
  Engine::RenderCommand::Initialize();
  Engine::Renderer2D::Initialize();
  Engine::Renderer::Initialize();

  Block::Initialize();
  Biome::Initialize();
  LOD::MeshData::Initialize();
  m_World.initialize();
}

void GameSandbox::onAttach()
{
}

void GameSandbox::onDetach()
{
}

void GameSandbox::onUpdate(Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  // EN_TRACE("fps: {0}", static_cast<int>(1.0f / timestep.sec()));

  Engine::RenderCommand::SetDepthWriting(true);
  Engine::RenderCommand::SetUseDepthOffset(false);
  Engine::RenderCommand::Clear(Float4(0.788f, 0.949f, 0.949f, 1.0f));

  Engine::Scene::OnUpdate(timestep);
  m_World.onUpdate(timestep);
}

void GameSandbox::onImGuiRender()
{
}

void GameSandbox::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));

  Engine::Scene::OnEvent(event);
  m_World.onEvent(event);
}

bool GameSandbox::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  static bool mouseEnabled = false;
  static bool wireFrameEnabled = false;
  static bool faceCullingEnabled = false;

  if (event.getKeyCode() == Key::Escape)
  {
    mouseEnabled = !mouseEnabled;
    mouseEnabled ? Engine::Application::Get().getWindow().enableCursor() : Engine::Application::Get().getWindow().disableCursor();
  }
  if (event.getKeyCode() == Key::F1)
  {
    wireFrameEnabled = !wireFrameEnabled;
    Engine::RenderCommand::SetWireFrame(wireFrameEnabled);
  }

  return false;
}
