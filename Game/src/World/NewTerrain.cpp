#include "GMpch.h"
#include "NewBiome.h"
#include "NewTerrain.h"
#include "World/Chunk/Chunk.h"

namespace terrain
{
  template<uSize N>
  static length_t evaluateShapingFunction(length_t x, const std::array<eng::math::Vec2, N>& controlPoints)
  {
    auto controlPointA = eng::algo::lowerBound(controlPoints, x, [](const eng::math::Vec2& controlPoint) { return controlPoint.x; });
    if (controlPointA == controlPoints.begin())
      return controlPointA->y;
    if (controlPointA == controlPoints.end())
      return controlPoints.back().y;

    auto controlPointB = std::prev(controlPointA);
    length_t t = (x - controlPointA->x) / (controlPointB->x - controlPointA->x);
    return std::lerp(controlPointA->y, controlPointB->y, t);
  }

  static length_t globalElevation(const eng::math::Vec2& pointXY)
  {
    length_t noiseValue = eng::math::normalizedOctaveNoise(pointXY, 6, 0.4f, 100_m, 0.5f);
    length_t elevation = eng::math::normalizedOctaveNoise(pointXY, 6, 0.3f, 1000_m, 0.5f);

    std::array<eng::math::Vec2, 7> elevationControlPoints = { { { -0.55,  -0.1  },
                                                                { -0.525, -0.95 },
                                                                { -0.5,   -1.0  },
                                                                { -0.475, -0.95 },
                                                                { -0.45,  -0.1  },
                                                                {  0.0,    0.0  },
                                                                {  1.0,    1.0  } } };
    std::array<eng::math::Vec2, 2> variationControlPoints = { { { -0.1,  0.1 },
                                                                {  0.5,  1.0 } } };

    length_t elevationControl = evaluateShapingFunction(elevation, elevationControlPoints);
    length_t variationControl = evaluateShapingFunction(elevationControl, variationControlPoints);
    return 100_m * elevationControl + 20_m * variationControl * (noiseValue + 1);
  }

  static biome::ID biomeSelection(length_t elevation)
  {
    if (elevation < -75_m)
      return biome::ID::Abyss;
    else if (elevation < -1_m)
      return biome::ID::Ocean;
    else if (elevation < 1_m)
      return biome::ID::Beach;
    else if (elevation < 75_m)
      return biome::ID::Default;
    else
      return biome::ID::Mountain;
  }

  static void heightMapStage(BlockArrayRect<length_t>& heightMap, const GlobalIndex& chunkIndex)
  {
    heightMap.forEach([&chunkIndex](BlockIndex2D surfaceIndex, length_t& surfaceHeight)
    {
      eng::math::Vec2 blockXY = Chunk::Length() * static_cast<eng::math::Vec2>(chunkIndex) + block::length() * static_cast<eng::math::Vec2>(surfaceIndex) + block::length() / 2;
      surfaceHeight = globalElevation(blockXY);
    });
  }

  static void fillStage(BlockArrayBox<block::Type>& composition, const BlockArrayRect<length_t>& heightMap, const GlobalIndex& chunkIndex)
  {
    eng::algo::fill(composition, block::ID::Air);
    heightMap.forEach([&composition, &chunkIndex](BlockIndex2D surfaceIndex, length_t surfaceHeight)
    {
      block::Type surfaceBlockType = getApproximateBlockType(surfaceHeight);

      i32 heightInBlocks = eng::arithmeticCastUnchecked<i32>(std::floor(surfaceHeight / block::length()));
      i32 surfaceBlockInChunk = heightInBlocks - Chunk::Size() * chunkIndex.k;
      if (surfaceBlockInChunk >= Chunk::Size())
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          composition[surfaceIndex.i][surfaceIndex.j][k] = block::ID::Stone;
      if (eng::withinBounds(surfaceBlockInChunk, 0, Chunk::Size()))
        while (surfaceBlockInChunk >= 0)
          composition[surfaceIndex.i][surfaceIndex.j][surfaceBlockInChunk--] = surfaceBlockType;
    });
  }

  length_t getApproximateElevation(const eng::math::Vec2& pointXY)
  {
    return globalElevation(pointXY);
  }

  block::Type getApproximateBlockType(length_t elevation)
  {
    switch (biomeSelection(elevation))
    {
      case biome::ID::Default: return block::ID::Grass;
      case biome::ID::Mountain: return block::ID::Snow;
      case biome::ID::Beach: return block::ID::Sand;
      case biome::ID::Ocean: return block::ID::Sand;
      case biome::ID::Abyss: return block::ID::Stone;
    }
    return block::ID::Null;
  }

  BlockArrayBox<block::Type> generateNew(const GlobalIndex& chunkIndex)
  {
    BlockArrayRect<length_t> heightMap(Chunk::Bounds2D(), eng::AllocationPolicy::ForOverwrite);
    BlockArrayBox<block::Type> composition(Chunk::Bounds(), eng::AllocationPolicy::ForOverwrite);

    heightMapStage(heightMap, chunkIndex);
    fillStage(composition, heightMap, chunkIndex);

    if (composition.filledWith(block::ID::Air))
      composition.clear();
    return composition;
  }
}