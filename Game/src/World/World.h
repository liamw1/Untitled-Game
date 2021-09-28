#pragma once
#include <Engine.h>
#include "Chunk.h"

namespace World
{
  void Initialize(const glm::vec3& initialPosition);
  void ShutDown();

  void OnUpdate(const glm::vec3& playerPosition);
};