#include "GMpch.h"
#include "GameSandbox.h"
#include "Player/Player.h"
#include "World/Biome.h"
#include "World/LOD.h"

GameSandbox::GameSandbox()
  : Layer("GameSandbox"),
    m_PrintFrameRate(false),
    m_PrintMinFrameRate(false),
    m_PrintPlayerPosition(false) {}

GameSandbox::~GameSandbox() = default;

void GameSandbox::onAttach()
{
}

void GameSandbox::onDetach()
{
}

void GameSandbox::onUpdate(eng::Timestep timestep)
{
  ENG_PROFILE_FUNCTION();

  if (m_PrintFrameRate || m_PrintMinFrameRate)
  {
    static constexpr i32 framerateWindowSize = 100;

    seconds frameTime = timestep.sec();
    m_FrameTimeWindow.push_front(frameTime);
    if (m_FrameTimeWindow.size() > framerateWindowSize)
      m_FrameTimeWindow.pop_back();

    if (m_PrintFrameRate)
    {
      seconds averageFrameTime = std::accumulate(m_FrameTimeWindow.begin(), m_FrameTimeWindow.end(), 0_s) / m_FrameTimeWindow.size();
      ENG_TRACE("FPS: {0}", eng::arithmeticCast<i32>(1 / averageFrameTime));
    }
    else
    {
      seconds maxFrameTime = *std::max_element(m_FrameTimeWindow.begin(), m_FrameTimeWindow.end());
      ENG_TRACE("Min FPS: {0}", eng::arithmeticCast<i32>(1 / maxFrameTime));
    }
  }
  else if (m_PrintPlayerPosition)
  {
    GlobalIndex position = eng::arithmeticUpcast<globalIndex_t>(Chunk::Size()) * player::originIndex() + GlobalIndex::ToIndex(player::position());
    ENG_TRACE("Position: {0}", position);
  }

  eng::scene::OnUpdate(timestep);
  m_World.onUpdate(timestep);
}

void GameSandbox::onEvent(eng::event::Event& event)
{
  event.dispatch(&GameSandbox::onKeyPress, this);

  eng::scene::OnEvent(event);
  m_World.onEvent(event);
}

bool GameSandbox::onKeyPress(eng::event::KeyPress& event)
{
  static bool mouseEnabled = false;
  static bool wireFrameEnabled = false;
  static bool faceCullingEnabled = false;

  if (event.keyCode() == eng::input::Key::Escape)
  {
    mouseEnabled = !mouseEnabled;
    mouseEnabled ? eng::Application::Get().window().enableCursor() : eng::Application::Get().window().disableCursor();
  }
  if (event.keyCode() == eng::input::Key::F1)
  {
    wireFrameEnabled = !wireFrameEnabled;
    eng::render::command::setWireFrame(wireFrameEnabled);
  }
  if (event.keyCode() == eng::input::Key::F2)
    m_PrintFrameRate = !m_PrintFrameRate;
  if (event.keyCode() == eng::input::Key::F3)
    m_PrintMinFrameRate = !m_PrintMinFrameRate;
  if (event.keyCode() == eng::input::Key::F4)
    m_PrintPlayerPosition = !m_PrintPlayerPosition;

  return false;
}
