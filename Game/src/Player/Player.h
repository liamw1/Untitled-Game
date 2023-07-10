#pragma once
#include "World/Chunk.h"

/*
  Represents the player.  The player's position (all the positions of all other
  geometry) is stored relative to an "origin chunk", which is the chunk the player
  is currently inside.
*/
namespace Player
{
  void Initialize(const GlobalIndex& chunkIndex, const Vec3& positionWithinChunk);

  void HandleDirectionalInput();

  void UpdatePosition(Timestep timestep);

  Vec3 Position();
  void SetPosition(const Vec3& position);

  Vec3 Velocity();
  void SetVelocity(const Vec3& velocity);

  Vec3 CameraPosition();
  Vec3 ViewDirection();

  GlobalIndex OriginIndex();

  length_t Width();
  length_t Height();
};