#include "GMpch.h"
#include <Engine/Core/EntryPoint.h>
#include "GameSandbox.h"

class GameApp : public Engine::Application
{
public:
  GameApp(Engine::ApplicationCommandLineArgs args)
    : Application("Game", args)
  {
    pushLayer(new GameSandbox());
  }

  ~GameApp()
  {
  }
};

Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args)
{
  return new GameApp(args);
}