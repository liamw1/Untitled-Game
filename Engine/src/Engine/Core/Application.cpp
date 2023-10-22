#include "ENpch.h"
#include "Application.h"
#include "Engine/Debug/Instrumentor.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Utilities/BindMember.h"

namespace eng
{
  const char* ApplicationCommandLineArgs::operator[](int index) const
  {
    ENG_CORE_ASSERT(debug::BoundsCheck(index, 0, count), "Index is out of bounds!");
    return args[index];
  }

  Application* Application::s_Instance = nullptr;

  Application::Application(const std::string& name, ApplicationCommandLineArgs args)
    : m_CommandLineArgs(args),
      m_Running(true),
      m_Minimized(false),
      m_LastFrameTime(std::chrono::steady_clock::now())
  {
    ENG_PROFILE_FUNCTION();

    ENG_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    m_Window = Window::Create(WindowProps(name));
    m_Window->setEventCallback(bindMemberFunction(&Application::onEvent, this));
    m_Window->setVSync(false);

    // m_ImGuiLayer = std::make_unique<ImGuiLayer>();
    // pushOverlay(m_ImGuiLayer);
  }

  Application::~Application() = default;

  void Application::run()
  {
    ENG_PROFILE_FUNCTION();

    while (m_Running)
    {
      ENG_PROFILE_SCOPE("Run Loop");

      std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
      Timestep timestep = Timestep(time - m_LastFrameTime);
      m_LastFrameTime = time;

      if (!m_Minimized)
        for (std::unique_ptr<Layer>& layer : m_LayerStack)
          layer->onUpdate(timestep);
      
      m_Window->onUpdate();
    }
  }

  void Application::onEvent(event::Event& event)
  {
    ENG_PROFILE_FUNCTION();

    event.dispatch(&Application::onWindowClose, this);
    event.dispatch(&Application::onWindowResize, this);

    for (LayerStack::reverse_iterator it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
    {
      if (event.handled())
        break;
      (*it)->onEvent(event);
    }
  }

  void Application::pushLayer(std::unique_ptr<Layer> layer)
  {
    m_LayerStack.pushLayer(std::move(layer));
  }

  void Application::close()
  {
    m_Running = false;
  }

  ImGuiLayer& Application::getImGuiLayer() { return *m_ImGuiLayer; }
  Window& Application::getWindow() { return *m_Window; }
  const ApplicationCommandLineArgs& Application::GetCommandLineArgs() const { return m_CommandLineArgs; }
  Application& Application::Get() { return *s_Instance; }

  bool Application::onWindowClose(event::WindowClose& /*event*/)
  {
    m_Running = false;
    return true;
  }

  bool Application::onWindowResize(event::WindowResize& event)
  {
    m_Minimized = event.width() == 0 || event.height() == 0;
    if (!m_Minimized)
      render::onWindowResize(event.width(), event.height());
    return false;
  }
}