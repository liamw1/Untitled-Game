#include <Engine.h>
#include <imgui/imgui.h>

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

  void onImGuiRender() override
  {
    ImGui::Begin("Test");
    ImGui::Text("Hellow World!");
    ImGui::End();
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