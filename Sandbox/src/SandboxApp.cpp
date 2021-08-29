#include <Engine.h>

class ExampleLayer : public Engine::Layer
{
public:
  ExampleLayer()
    : Layer("Example") {}

  void onUpdate() override
  {
    EN_INFO("ExampleLayer::Update");
  }

  void onEvent(Engine::Event& event) override
  {
    EN_TRACE("{0}", event);
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