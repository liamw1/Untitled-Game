#include <Engine.h>
#include <Engine/Core/EntryPoint.h>

class GameApp : public Engine::Application
{
public:
  GameApp()
  {
  }

  ~GameApp()
  {
  }
};

Engine::Application* Engine::CreateApplication()
{
  return new GameApp();
}