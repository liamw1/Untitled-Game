#pragma once
#include "BlockIDs.h"
#include "World/Indexing.h"
#include "Util/CompoundType.h"
#include <Engine.h>

namespace Block
{
  using CompoundType = ::CompoundType<Type, 4, Block::Type::Null>;

  void Initialize();
  std::shared_ptr<Engine::TextureArray> GetTextureArray();

  Texture GetTexture(Type block, Direction face);

  bool HasTransparency(Texture texture);
  bool HasTransparency(Type block);
  bool HasCollision(Type block);

  constexpr length_t Length() { return 0.5_m; }
  constexpr float LengthF() { return static_cast<float>(Length()); }
};