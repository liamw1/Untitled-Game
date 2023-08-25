#include "ENpch.h"
#include "Application.h"
#include "Core.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Debug/Instrumentor.h"

namespace Engine
{
  const char* ApplicationCommandLineArgs::operator[](int index) const
  {
    EN_CORE_ASSERT(index < count, "Index is out of bounds!");
    return args[index];
  }

  Application* Application::s_Instance = nullptr;

  Application::Application(const std::string& name, ApplicationCommandLineArgs args)
    : m_CommandLineArgs(args),
      m_Running(true),
      m_Minimized(false),
      m_LastFrameTime(std::chrono::steady_clock::now())
  {
    EN_PROFILE_FUNCTION();

    EN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
    s_Instance = this;

    m_Window = Window::Create(WindowProps(name));
    m_Window->setEventCallback(EN_BIND_EVENT_FN(onEvent));
    m_Window->setVSync(true);

    m_LayerStack = std::make_unique<LayerStack>();
    // m_ImGuiLayer = new ImGuiLayer();
    // pushOverlay(m_ImGuiLayer);
  }

  Application::~Application()
  {
    EN_PROFILE_FUNCTION();

    Renderer::Shutdown();
  }

  void Application::run()
  {
    EN_PROFILE_FUNCTION();

    while (m_Running)
    {
      EN_PROFILE_SCOPE("Run Loop");

      auto time = std::chrono::steady_clock::now();
      Timestep timestep = Timestep(time - m_LastFrameTime);
      m_LastFrameTime = time;

      if (!m_Minimized)
      {
        for (Layer* layer : *m_LayerStack)
          layer->onUpdate(timestep);

        // m_ImGuiLayer->begin();
        // for (Layer* layer : *m_LayerStack)
        //   layer->onImGuiRender();
        // m_ImGuiLayer->end();
      }
      
      m_Window->onUpdate();
    }
  }

  void Application::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<WindowCloseEvent>(EN_BIND_EVENT_FN(onWindowClose));
    dispatcher.dispatch<WindowResizeEvent>(EN_BIND_EVENT_FN(onWindowResize));

    for (auto it = m_LayerStack->rbegin(); it != m_LayerStack->rend(); ++it)
    {
      if (event.handled)
        break;
      (*it)->onEvent(event);
    }
  }

  void Application::pushLayer(Layer* layer)
  {
    m_LayerStack->pushLayer(layer);
    layer->onAttach();
  }

  void Application::pushOverlay(Layer* layer)
  {
    m_LayerStack->pushOverlay(layer);
    layer->onAttach();
  }

  void Application::close()
  {
    m_Running = false;
  }

  ImGuiLayer* Application::getImGuiLayer() { return m_ImGuiLayer; }
  Window& Application::getWindow() { return *m_Window; }
  const ApplicationCommandLineArgs& Application::GetCommandLineArgs() const { return m_CommandLineArgs; }
  Application& Application::Get() { return *s_Instance; }

  bool Application::onWindowClose(WindowCloseEvent& /*event*/)
  {
    m_Running = false;
    return true;
  }

  bool Application::onWindowResize(WindowResizeEvent& event)
  {
    if (event.width() == 0 || event.height() == 0)
    {
      m_Minimized = true;
      return false;
    }
    m_Minimized = false;
    Renderer::OnWindowResize(event.width(), event.height());

    return false;
  }
}