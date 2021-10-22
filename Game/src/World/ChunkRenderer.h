#pragma once
#include "Block/Block.h"

class Chunk;

/*
  A specialized renderer optimized for chunk rendering.
*/
namespace ChunkRenderer
{
  /*
    Setup that only needs to be done once.
  */
  void Initialize();

  /*
    Setup that only needs to be done once per frame.
  */
  void BeginScene(const Engine::Camera& camera);
  void EndScene();
  
  /*
    Draws mesh of given chunk.
  */
  void DrawChunk(const Chunk* chunk);
};