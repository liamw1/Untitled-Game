#pragma once
#include "World/Chunk.h"

/*
  Represents the player.  The player's position (all the positions of all other
  geometry) is stored relative to an "origin chunk", which is the chunk the player
  is currently inside.
*/
namespace player
{
  void initialize(const GlobalIndex& chunkIndex, const eng::math::Vec3& positionWithinChunk);

  void handleDirectionalInput();

  void updatePosition(eng::Timestep timestep);

  eng::math::Vec3 position();
  void setPosition(const eng::math::Vec3& position);

  eng::math::Vec3 velocity();
  void setVelocity(const eng::math::Vec3& velocity);

  eng::math::Vec3 cameraPosition();
  eng::math::Vec3 viewDirection();

  GlobalIndex originIndex();

  length_t width();
  length_t height();
};