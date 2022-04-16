#pragma once
#include "Application.h"
#include "Engine/Threading/Threads.h"
#include "Engine/Debug/Instrumentor.h"

#ifdef EN_PLATFORM_WINDOWS

extern Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
  Threads::SetMainThreadID(std::this_thread::get_id());

  Engine::Log::Initialize();

  EN_PROFILE_BEGIN_SESSION("Startup", "EngineProfile-Startup.json");
  Engine::Application* app = Engine::CreateApplication({ argc, argv });
  EN_PROFILE_END_SESSION();

  EN_PROFILE_BEGIN_SESSION("Runtime", "EngineProfile-Runtime.json");
  app->run();
  EN_PROFILE_END_SESSION();

  EN_PROFILE_BEGIN_SESSION("Shutdown", "EngineProfile-Shutdown.json");
  delete app;
  EN_PROFILE_END_SESSION();
}

#endif