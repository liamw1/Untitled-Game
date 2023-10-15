#pragma once
#include "Application.h"
#include "Engine/Debug/Instrumentor.h"
#include "Engine/Threads/Threads.h"

#ifdef EN_PLATFORM_WINDOWS

extern eng::Application* eng::createApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
  eng::threads::setAsMainThread();

  EN_PROFILE_BEGIN_SESSION("Startup", "EngineProfile-Startup.json");
  eng::Application* app = eng::createApplication({ argc, argv });
  EN_PROFILE_END_SESSION();

  EN_PROFILE_BEGIN_SESSION("Runtime", "EngineProfile-Runtime.json");
  app->run();
  EN_PROFILE_END_SESSION();

  EN_PROFILE_BEGIN_SESSION("Shutdown", "EngineProfile-Shutdown.json");
  delete app;
  EN_PROFILE_END_SESSION();
}

#endif