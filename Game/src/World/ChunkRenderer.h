#pragma once
#include "Block/Block.h"

class Chunk;

/*
  A specialized renderer optimized for chunk rendering.
*/
namespace ChunkRenderer
{
  void Initialize();

  void BeginScene(Engine::Camera& camera);
  void EndScene();

  void DrawChunk(const Chunk* chunk);
};