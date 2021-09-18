#include "Sandbox2D.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

template<typename Fn>
class Timer
{
public:
  Timer(const char* name, Fn&& callback)
    : m_Stopped(false),
      m_Name(name), 
      m_StartTimepoint(std::chrono::steady_clock::now()),
      m_Callback(callback)
  {
  }
  ~Timer()
  {
    if (!m_Stopped)
      stop();
  }

  void stop()
  {
    auto endTimepoint = std::chrono::steady_clock::now();
    std::chrono::duration<uint64_t, std::nano> duration = endTimepoint - m_StartTimepoint;
    m_Stopped = true;
    m_Callback({ m_Name, (float)duration.count() / 1e6f });
  }

private:
  bool m_Stopped;
  const char* m_Name;
  std::chrono::steady_clock::time_point m_StartTimepoint;
  Fn m_Callback;
};

#define PROFILE_SCOPE(name) Timer timer##__LINE__(name, [&](ProfileResult profileReuslt) { m_ProfileResults.emplace_back(profileReuslt); })

Sandbox2D::Sandbox2D()
  : Layer("Sandbox2D"),
    m_CameraController(1280.0f / 720.0f, true)
{
}

void Sandbox2D::onAttach()
{
  m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");
}

void Sandbox2D::onDetach()
{
}

void Sandbox2D::onUpdate(std::chrono::duration<uint64_t, std::nano> timestep)
{
  PROFILE_SCOPE("Sandbox2D::onUpdate");

  const float dt = (float)timestep.count() / 1e9f;  // Time between frames in seconds
  
  {
    PROFILE_SCOPE("CameraController::onUpdate");
    m_CameraController.onUpdate(timestep);
  }

  {
    PROFILE_SCOPE("Renderer Prep");
    Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });
  }

  {
    PROFILE_SCOPE("Renderer Draw");
    Engine::Renderer2D::BeginScene(m_CameraController.getCamera());
    Engine::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
    Engine::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
    Engine::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 10.0f, 10.0f }, m_CheckerboardTexture);
    Engine::Renderer2D::EndScene();
  }
}

void Sandbox2D::onImGuiRender()
{
  ImGui::Begin("Settings");
  ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));

  for (auto& result : m_ProfileResults)
  {
    char label[50];
    strcpy_s(label, "%.3fms  ");
    strcat_s(label, result.name);
    ImGui::Text(label, result.time);
  }
  m_ProfileResults.clear();

  ImGui::End();
}

void Sandbox2D::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}
