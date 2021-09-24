#include <Engine.h>
#include <Engine/Core/EntryPoint.h>

#include "Sandbox2D.h"
#include "Sandbox3D.h"

class Sandbox : public Engine::Application
{
public:
  Sandbox()
  {
    // pushLayer(new Sandbox2D());
    pushLayer(new Sandbox3D());
  }

  ~Sandbox()
  {
  }
};

Engine::Application* Engine::CreateApplication()
{
  return new Sandbox();
}