#include "GMpch.h"
#include "GameSandbox.h"
#include "World/World.h"
#include "World/LOD.h"

GameSandbox::GameSandbox()
  : Layer("GameSandbox")
{
  Player::Initialize(GlobalIndex(0, 0, 1), Block::Length() * Vec3(16.0));
  Engine::RenderCommand::Initialize();
  Engine::Renderer::Initialize();

  Block::Initialize();

  Shared<Engine::TextureArray> textureArray = Engine::TextureArray::Create(16, 128);
  for (Block::Texture texture : Block::TextureIterator())
    textureArray->addTexture(Block::GetTexturePath(texture));

  Chunk::Initialize(textureArray);
  LOD::Initialize(textureArray);
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
    Engine::RenderCommand::WireFrameToggle(wireFrameEnabled);
  }
  if (event.getKeyCode() == Key::F2)
  {
    faceCullingEnabled = !faceCullingEnabled;
    Engine::RenderCommand::FaceCullToggle(faceCullingEnabled);
  }

  return false;
}
