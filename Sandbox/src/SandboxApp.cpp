#include <Engine.h>

class ExampleLayer : public Engine::Layer
{
public:
  ExampleLayer()
    : Layer("Example") {}

  void onUpdate() override
  {
  }

  void onImGuiRender() override
  {
  }
};

class Sandbox : public Engine::Application
{
public:
  Sandbox()
  {
    pushLayer(new ExampleLayer());
  }

  ~Sandbox()
  {

  }
};

Engine::Application* Engine::CreateApplication()
{
  return new Sandbox();
}