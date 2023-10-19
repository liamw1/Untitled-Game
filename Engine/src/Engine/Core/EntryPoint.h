#pragma once
#include "Application.h"
#include "Engine/Debug/Instrumentor.h"
#include "Engine/Threads/Threads.h"

#ifdef ENG_PLATFORM_WINDOWS

extern eng::Application* eng::createApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
  eng::threads::setAsMainThread();

  ENG_PROFILE_BEGIN_SESSION("Startup", "EngineProfile-Startup.json");
  eng::Application* app = eng::createApplication({ argc, argv });
  ENG_PROFILE_END_SESSION();

  ENG_PROFILE_BEGIN_SESSION("Runtime", "EngineProfile-Runtime.json");
  app->run();
  ENG_PROFILE_END_SESSION();

  ENG_PROFILE_BEGIN_SESSION("Shutdown", "EngineProfile-Shutdown.json");
  delete app;
  ENG_PROFILE_END_SESSION();
}

#endif