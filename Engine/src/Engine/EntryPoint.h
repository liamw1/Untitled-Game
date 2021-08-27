#pragma once
#include "Application.h"
#include "Log.h"

#ifdef EN_PLATFORM_WINDOWS

extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv)
{
  Engine::Log::Init();
  EN_CORE_WARN("Initialized Log!");
  EN_INFO("Hello!");

  Engine::Application* app = Engine::CreateApplication();
  app->run();
  delete app;
}

#endif