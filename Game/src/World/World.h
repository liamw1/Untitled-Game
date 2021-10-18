#pragma once
#include "Chunk.h"

namespace World
{
  void Initialize(const glm::vec3& initialPosition);
  void ShutDown();

  void OnUpdate(const Engine::Camera& playerCamera);

  void OnEvent(Engine::Event& event);
};