#include <Engine.h>

class ExampleLayer : public Engine::Layer
{
public:
  ExampleLayer()
    : Layer("Example") {}

  void onUpdate() override
  {
    if (Engine::Input::IsKeyPressed(Key::Tab))
      EN_TRACE("Tab key is pressed!");
  }

  void onEvent(Engine::Event& event) override
  {
  }
};

class Sandbox : public Engine::Application
{
public:
  Sandbox()
  {
    pushLayer(new ExampleLayer());
    pushOverlay(new Engine::ImGuiLayer());
  }

  ~Sandbox()
  {

  }
};

Engine::Application* Engine::CreateApplication()
{
  return new Sandbox();
}