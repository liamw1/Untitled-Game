#include "SBpch.h"
#include "Sandbox2D.h"
#include "Sandbox3D.h"
#include <Engine/Core/EntryPoint.h>

class Sandbox : public Engine::Application
{
public:
  Sandbox(Engine::ApplicationCommandLineArgs args)
    : Application("Sandbox", args)
  {
    pushLayer(new Sandbox2D());
    // pushLayer(new Sandbox3D());
  }

  ~Sandbox()
  {
  }
};

Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args)
{
  return new Sandbox(args);
}