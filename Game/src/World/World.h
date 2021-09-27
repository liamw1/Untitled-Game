#pragma once
#include <Engine.h>
#include "Chunk.h"

namespace World
{
  void Initialize();
  void ShutDown();

  void OnUpdate(const glm::vec3& playerPosition);
};