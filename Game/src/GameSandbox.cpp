#include "GMpch.h"
#include "GameSandbox.h"
#include "Player/Player.h"
#include "World/Biome.h"
#include "World/LOD.h"

GameSandbox::GameSandbox()
  : Layer("GameSandbox"),
    m_PrintFrameRate(false),
    m_PrintMinFrameRate(false),
    m_PrintPlayerPosition(false)
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

GameSandbox::~GameSandbox() = default;

void GameSandbox::onAttach()
{
}

void GameSandbox::onDetach()
{
}

void GameSandbox::onUpdate(Engine::Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  if (m_PrintFrameRate || m_PrintMinFrameRate)
  {
    static constexpr int framerateWindowSize = 100;

    float frameTime = timestep.sec();
    m_FrameTimeWindow.push_front(frameTime);
    if (m_FrameTimeWindow.size() > framerateWindowSize)
      m_FrameTimeWindow.pop_back();

    if (m_PrintFrameRate)
    {
      float averageFrameTime = std::accumulate(m_FrameTimeWindow.begin(), m_FrameTimeWindow.end(), 0.0f) / m_FrameTimeWindow.size();
      EN_TRACE("FPS: {0}", static_cast<int>(1.0f / averageFrameTime));
    }
    else
    {
      float maxFrameTime = *std::max_element(m_FrameTimeWindow.begin(), m_FrameTimeWindow.end());
      EN_TRACE("Min FPS: {0}", static_cast<int>(1.0f / maxFrameTime));
    }
  }
  else if (m_PrintPlayerPosition)
  {
    GlobalIndex position = static_cast<globalIndex_t>(Chunk::Size()) * Player::OriginIndex() + GlobalIndex::ToIndex(Player::Position());
    EN_TRACE("Position: {0}", position);
  }

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

  if (event.keyCode() == Key::Escape)
  {
    mouseEnabled = !mouseEnabled;
    mouseEnabled ? Engine::Application::Get().getWindow().enableCursor() : Engine::Application::Get().getWindow().disableCursor();
  }
  if (event.keyCode() == Key::F1)
  {
    wireFrameEnabled = !wireFrameEnabled;
    Engine::RenderCommand::SetWireFrame(wireFrameEnabled);
  }
  if (event.keyCode() == Key::F2)
    m_PrintFrameRate = !m_PrintFrameRate;
  if (event.keyCode() == Key::F3)
    m_PrintMinFrameRate = !m_PrintMinFrameRate;
  if (event.keyCode() == Key::F4)
    m_PrintPlayerPosition = !m_PrintPlayerPosition;

  return false;
}
