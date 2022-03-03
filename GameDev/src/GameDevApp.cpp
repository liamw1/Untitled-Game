#include "GDpch.h"
#include <Engine/Core/EntryPoint.h>
#include "EditorLayer.h"

namespace Engine
{
  class GameDev : public Application
  {
  public:
    GameDev()
      : Application("GameDev")
    {
      pushLayer(new EditorLayer());
    }

    ~GameDev()
    {
    }
  };

  Application* CreateApplication()
  {
    return new GameDev();
  }
}