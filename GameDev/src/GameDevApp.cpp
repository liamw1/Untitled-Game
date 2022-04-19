#include "GDpch.h"
#include "EditorLayer.h"
#include <Engine/Core/EntryPoint.h>

namespace Engine
{
  class GameDev : public Application
  {
  public:
    GameDev(ApplicationCommandLineArgs args)
      : Application("GameDev", args)
    {
      pushLayer(new EditorLayer());
    }

    ~GameDev()
    {
    }
  };

  Application* CreateApplication(ApplicationCommandLineArgs args)
  {
    return new GameDev(args);
  }
}