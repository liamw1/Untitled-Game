#include "GMpch.h"
#include "GameSandbox.h"
#include <Engine/Core/EntryPoint.h>

class GameApp : public eng::Application
{
public:
  GameApp(eng::ApplicationCommandLineArgs args)
    : Application("Game", args) { pushLayer(new GameSandbox()); }

  ~GameApp() = default;
};

eng::Application* eng::createApplication(ApplicationCommandLineArgs args)
{
  return new GameApp(args);
}