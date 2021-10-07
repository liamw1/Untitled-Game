#include "GMpch.h"
#include <Engine/Core/EntryPoint.h>
#include "GameSandbox.h"

class GameApp : public Engine::Application
{
public:
  GameApp()
  {
    pushLayer(new GameSandbox());
  }

  ~GameApp()
  {
  }
};

Engine::Application* Engine::CreateApplication()
{
  return new GameApp();
}