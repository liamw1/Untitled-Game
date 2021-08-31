#pragma once
#include "Application.h"

#ifdef EN_PLATFORM_WINDOWS

extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv)
{
  Engine::Log::Initialize();
  EN_CORE_WARN("Initialized Log!");

  Engine::Application* app = Engine::CreateApplication();
  app->run();
  delete app;
}

#endif