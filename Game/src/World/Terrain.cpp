#include "GMpch.h"
#include "Terrain.h"
#include "TerrainProperties.h"
#include "World/Chunk/Chunk.h"
#include "World/Biome/BiomeTable.h"

namespace terrain
{
  static constexpr int c_TerrainMaxAmplitude = 100;
  static constexpr eng::EnumArray<biome::PropertyVector, biome::ID> c_BiomeProperties =
    { { biome::ID::Default,  { { biome::Property::Elevation,  0.3_m } } },
      { biome::ID::Mountain, { { biome::Property::Elevation,  0.9_m } } },
      { biome::ID::Beach,    { { biome::Property::Elevation,  0.1_m } } },
      { biome::ID::Ocean,    { { biome::Property::Elevation, -0.2_m } } },
      { biome::ID::Abyss,    { { biome::Property::Elevation, -0.9_m } } } };

  static biome::Table createBiomeTable()
  {
    biome::Table table;
    for (biome::ID biome : eng::EnumIterator<biome::ID>())
      table.addBiome(biome, c_BiomeProperties[biome]);
    return table;
  }
  static biome::Table s_BiomeTable = createBiomeTable();

  static constexpr eng::math::Vec2 calculateBlockXY(const GlobalIndex& chunkIndex, BlockIndex2D surfaceIndex)
  {
    return Chunk::Length() * static_cast<eng::math::Vec2>(chunkIndex) + block::length() * static_cast<eng::math::Vec2>(surfaceIndex) + block::length() / 2;
  }

  template<uSize N>
  static constexpr length_t evaluateShapingFunction(length_t x, const std::array<eng::math::Vec2, N>& controlPoints)
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

  static length_t baseElevation(length_t elevationProperty, const eng::math::Vec2& pointXY)
  {
    static constexpr std::array<eng::math::Vec2, 7> elevationControlPoints = { { { -0.55,  -0.1  },
                                                                                 { -0.525, -0.95 },
                                                                                 { -0.5,   -1.0  },
                                                                                 { -0.475, -0.95 },
                                                                                 { -0.45,  -0.1  },
                                                                                 {  0.0,    0.0  },
                                                                                 {  1.0,    1.0  } } };
    static constexpr std::array<eng::math::Vec2, 2> variationControlPoints = { { { -0.1,  0.1 },
                                                                                 {  0.5,  1.0 } } };

    length_t elevationControl = evaluateShapingFunction(elevationProperty, elevationControlPoints);
    length_t variationControl = evaluateShapingFunction(elevationControl, variationControlPoints);
    length_t noiseValue = eng::math::normalizedOctaveNoise(pointXY, 6, 0.4f, 1000_m, 0.5f) + 1;
    return c_TerrainMaxAmplitude * elevationControl + 0.2_m * c_TerrainMaxAmplitude * variationControl * noiseValue;
  }

  static BlockArrayRect<biome::PropertyVector> terrainPropertyStage(const GlobalIndex& chunkIndex)
  {
    BlockArrayRect<biome::PropertyVector> properties(Chunk::Bounds2D(), eng::AllocationPolicy::ForOverwrite);
    properties.populate([&chunkIndex](BlockIndex2D surfaceIndex) -> biome::PropertyVector
    {
      return { { biome::Property::Elevation, terrain::elevationProperty(calculateBlockXY(chunkIndex, surfaceIndex)) } };
    });
    return properties;
  }

  static BlockArrayRect<length_t> heightMapStage(const BlockArrayRect<biome::PropertyVector>& terrainProperties, const GlobalIndex& chunkIndex)
  {
    BlockArrayRect<length_t> heightMap(Chunk::Bounds2D(), eng::AllocationPolicy::ForOverwrite);
    heightMap.populate([&terrainProperties, &chunkIndex](BlockIndex2D surfaceIndex)
    {
      eng::math::Vec2 blockXY = calculateBlockXY(chunkIndex, surfaceIndex);
      return baseElevation(elevationProperty(blockXY), blockXY);
    });
    return heightMap;
  }

  static BlockArrayBox<block::Type> fillStage(const BlockArrayRect<biome::PropertyVector>& terrainProperties, const BlockArrayRect<length_t>& heightMap, const GlobalIndex& chunkIndex)
  {
    BlockArrayBox<block::Type> composition(Chunk::Bounds(), eng::AllocationPolicy::ForOverwrite);

    eng::algo::fill(composition, block::ID::Air);
    heightMap.forEach([&composition, &terrainProperties, &chunkIndex](BlockIndex2D surfaceIndex, length_t surfaceHeight)
    {
      i32 heightInBlocks = eng::arithmeticCastUnchecked<i32>(std::floor(surfaceHeight / block::length()));
      i32 surfaceBlockInChunk = heightInBlocks - Chunk::Size() * chunkIndex.k;
      if (surfaceBlockInChunk >= Chunk::Size())
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          composition[surfaceIndex.i][surfaceIndex.j][k] = block::ID::Stone;
      if (eng::withinBounds(surfaceBlockInChunk, 0, Chunk::Size()))
      {
        biome::ID biome = biomeAt(terrainProperties(surfaceIndex));
        block::Type surfaceType = getApproximateBlockType(biome);
        block::Type soilType = surfaceType == block::ID::Grass ? block::ID::Dirt : surfaceType;

        blockIndex_t k = surfaceBlockInChunk;
        composition[surfaceIndex.i][surfaceIndex.j][k--] = surfaceType;
        while (k >= std::max(surfaceBlockInChunk - 3, 0))
          composition[surfaceIndex.i][surfaceIndex.j][k--] = soilType;
        while (k >= 0)
          composition[surfaceIndex.i][surfaceIndex.j][k--] = block::ID::Stone;
      }
    });
    return composition;
  }

  biome::PropertyVector terrainPropertiesAt(const eng::math::Vec2& pointXY)
  {
    return { { biome::Property::Elevation, terrain::elevationProperty(pointXY) } };
  }

  biome::ID biomeAt(const biome::PropertyVector& terrainProperties)
  {
    return s_BiomeTable.at(terrainProperties);
  }

  block::Type getApproximateBlockType(biome::ID biome)
  {
    static constexpr eng::EnumArray<block::Type, biome::ID> defaultSurfaceBlocks =
    { { biome::ID::Default,   block::ID::Grass  },
      { biome::ID::Mountain,  block::ID::Snow   },
      { biome::ID::Beach,     block::ID::Sand   },
      { biome::ID::Ocean,     block::ID::Sand   },
      { biome::ID::Abyss,     block::ID::Stone  } };
    return defaultSurfaceBlocks[biome];
  }

  length_t getApproximateElevation(const eng::math::Vec2& pointXY)
  {
    biome::PropertyVector terrainProperties = terrainPropertiesAt(pointXY);
    return baseElevation(terrainProperties[biome::Property::Elevation], pointXY);
  }

  BlockArrayBox<block::Type> generateNew(const GlobalIndex& chunkIndex)
  {
    BlockArrayRect<biome::PropertyVector> terrainProperties = terrainPropertyStage(chunkIndex);
    BlockArrayRect<length_t> heightMap = heightMapStage(terrainProperties, chunkIndex);
    BlockArrayBox<block::Type> composition = fillStage(terrainProperties, heightMap, chunkIndex);

    if (composition.filledWith(block::ID::Air))
      composition.clear();
    return composition;
  }
}