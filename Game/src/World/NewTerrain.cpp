#include "GMpch.h"
#include "NewTerrain.h"
#include "World/Chunk/Chunk.h"

namespace terrain
{
  static constexpr block::Type defaultBlockType(block::ID::Stone);

  static length_t evaluateShapingFunction(length_t x, const std::vector<eng::math::Vec2>& controlPoints)
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
    length_t noiseValue = eng::math::normalizedOctaveNoise(pointXY, 8, 0.4f, 100_m, 0.5f);
    length_t elevation = eng::math::normalizedOctaveNoise(pointXY, 8, 0.5f, 200_m, 0.6f);

    std::vector<eng::math::Vec2> controlPoints;
    controlPoints.push_back({ -0.5, 0.5 });
    controlPoints.push_back({ 0.0, 0.1 });
    controlPoints.push_back({ 1.0, 1.0 });

    return 100_m * evaluateShapingFunction(elevation, controlPoints) * noiseValue;
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
      i32 heightInBlocks = eng::arithmeticCastUnchecked<i32>(std::floor(surfaceHeight / block::length()));
      i32 surfaceBlockInChunk = heightInBlocks - Chunk::Size() * chunkIndex.k;
      if (surfaceBlockInChunk >= Chunk::Size())
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          composition[surfaceIndex.i][surfaceIndex.j][k] = defaultBlockType;
      if (eng::withinBounds(surfaceBlockInChunk, 0, Chunk::Size()))
        while (surfaceBlockInChunk >= 0)
          composition[surfaceIndex.i][surfaceIndex.j][surfaceBlockInChunk--] = defaultBlockType;
    });
  }

  length_t getApproximateElevation(const eng::math::Vec2& pointXY)
  {
    return globalElevation(pointXY);
  }

  block::Type getApproximateBlockType(const eng::math::Vec2& pointXY)
  {
    return defaultBlockType;
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