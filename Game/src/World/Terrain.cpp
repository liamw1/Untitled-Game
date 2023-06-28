#include "GMpch.h"
#include "Terrain.h"
#include "ChunkContainer.h"
#include "Util/Noise.h"
#include "Util/Util.h"
#include "Util/LRUCache.h"
#include "Player/Player.h"

Terrain::CompoundSurfaceData Terrain::CompoundSurfaceData::operator+(const CompoundSurfaceData& other) const
{
  CompoundSurfaceData sum = *this;
  sum.m_Elevation += other.m_Elevation;
  sum.m_Components += other.m_Components;

  return sum;
}

Terrain::CompoundSurfaceData Terrain::CompoundSurfaceData::operator*(float x) const
{
  CompoundSurfaceData result = *this;
  result.m_Elevation *= x;
  result.m_Components *= x;

  return result;
}

std::array<int, 2> Terrain::CompoundSurfaceData::getTextureIndices() const
{
  std::array<int, 2> textureIndices{};

  textureIndices[0] = static_cast<int>(Block::GetTexture(m_Components[0].type, Direction::Top));
  textureIndices[1] = static_cast<int>(Block::GetTexture(m_Components[1].type, Direction::Top));

  return textureIndices;
}



static constexpr length_t c_LargestNoiseScale = 1024 * Block::Length();
static constexpr float c_NoiseLacunarity = 2.0f;

static constexpr int c_MaxCompoundBiomes = 4;
static constexpr int c_BiomeRegionSize = 8;
static constexpr int c_RegionRadius = 1;
static constexpr int c_RegionWidth = 2 * c_RegionRadius + 1;

using NoiseSamples = Array2D<Noise::OctaveNoiseData<Biome::LocalElevationOctaves()>, Chunk::Size()>;
using CompoundBiome = CompoundType<Biome::Type, c_MaxCompoundBiomes, Biome::Type::Null>;
using BiomeData = Array2D<CompoundBiome, Chunk::Size()>;

struct SurfaceData
{
  NoiseSamples noiseSamples = AllocateArray2D<Noise::OctaveNoiseData<Biome::LocalElevationOctaves()>, Chunk::Size()>();
  BiomeData biomeData = AllocateArray2D<CompoundBiome, Chunk::Size()>();
};

static constexpr int c_CacheSize = (2 * c_UnloadDistance + 5) * (2 * c_UnloadDistance + 5);
static LRUCache<SurfaceMapIndex, SurfaceData> s_SurfaceDataCache(c_CacheSize);
static std::mutex s_Mutex;

// From https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
static uint32_t hash(uint32_t n)
{
  n = ((n >> 16) ^ n) * 0x45d9f3b;
  n = ((n >> 16) ^ n) * 0x45d9f3b;
  return (n >> 16) ^ n;
}

// Returns a random float in the range [0.0, 1.0] based determinisitically on the input n
static float random(uint32_t n)
{
  return static_cast<float>(hash(n)) / std::numeric_limits<uint32_t>::max();
}

// Returns a biome type based determinisitically on the input n
static Biome::Type randomBiome(uint32_t n)
{
  return static_cast<Biome::Type>(hash(n) % Biome::Count());
}

static std::pair<Biome::Type, Float2> getRegionVoronoiPoint(const SurfaceMapIndex& regionIndex)
{
  uint32_t key = std::hash<SurfaceMapIndex>{}(regionIndex);
  float r = 0.5f * random(key);
  float theta = 2 * Constants::PI * random(hash(key));

  Biome::Type biomeType = randomBiome(key);
  Float2 relativeLocation(r * std::cos(theta), r * std::sin(theta));
  return { biomeType, relativeLocation };
}

static CompoundBiome getBiomeData(const Vec2& surfaceLocation)
{
  using WeightedBiome = CompoundBiome::Component;

  SurfaceMapIndex queryRegionIndex = SurfaceMapIndex::ToIndex(surfaceLocation / Chunk::Size() / c_BiomeRegionSize);
  Float2 queryLocationRelativeToQueryRegion = surfaceLocation / Chunk::Size() / c_BiomeRegionSize - static_cast<Vec2>(queryRegionIndex);

  std::array<WeightedBiome, c_RegionWidth* c_RegionWidth> nearbyBiomes;
  for (globalIndex_t i = -c_RegionRadius; i <= c_RegionRadius; ++i)
    for (globalIndex_t j = -c_RegionRadius; j <= c_RegionRadius; ++j)
    {
      SurfaceMapIndex regionIndex = queryRegionIndex + SurfaceMapIndex(i, j);
      Float2 regionCenterRelativeToQueryRegion(i + 0.5f, j + 0.5f);

      auto [biomeType, voronoiPointPerturbation] = getRegionVoronoiPoint(regionIndex);
      Float2 voronoiPointRelativeToQueryRegion = regionCenterRelativeToQueryRegion + voronoiPointPerturbation;
      float distance = glm::distance(queryLocationRelativeToQueryRegion, voronoiPointRelativeToQueryRegion);
      float biomeWeight = std::expf(-32 * distance * distance);

      int index = c_RegionWidth * (i + c_RegionRadius) + j + c_RegionRadius;
      nearbyBiomes[index] = { biomeType, biomeWeight };
    }

  // Combine components of same type
  std::sort(nearbyBiomes.begin(), nearbyBiomes.end(), [](const WeightedBiome& biomeA, const WeightedBiome& biomeB) { return static_cast<int>(biomeA.type) < static_cast<int>(biomeB.type); });
  int lastUniqueElementIndex = 0;
  for (int i = 1; i < nearbyBiomes.size(); ++i)
  {
    if (nearbyBiomes[i].type == nearbyBiomes[i - 1].type)
    {
      nearbyBiomes[lastUniqueElementIndex].weight += nearbyBiomes[i].weight;
      nearbyBiomes[i] = { Biome::Type::Null, 0.0f };
    }
    else
      lastUniqueElementIndex = i;
  }

  // Sort biomes by weight and create compound type
  std::sort(nearbyBiomes.begin(), nearbyBiomes.end(), [](const WeightedBiome& biomeA, const WeightedBiome& biomeB) { return biomeA.weight > biomeB.weight; });
  return CompoundBiome(nearbyBiomes);
}

static const SurfaceData& getSurfaceData(const GlobalIndex& chunkIndex)
{
  SurfaceMapIndex mapIndex = static_cast<SurfaceMapIndex>(chunkIndex);
  auto cachePosition = s_SurfaceDataCache.find(mapIndex);
  if (cachePosition == s_SurfaceDataCache.end())
  {
    // NOTE: Voronoi points could be calculated here instead of each time getBiomeData is called

    // Generate surface data
    SurfaceData surfaceData{};
    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        Vec2 blockXY = Chunk::Length() * static_cast<Vec2>(mapIndex) + Block::Length() * (Vec2(i, j) + Vec2(0.5));

        surfaceData.noiseSamples[i][j] = Noise::OctaveNoise2D<Biome::LocalElevationOctaves()>(blockXY, 1_m / c_LargestNoiseScale, c_NoiseLacunarity);
        surfaceData.biomeData[i][j] = getBiomeData(blockXY);
      }

    auto [insertionPosition, insertionSuccess] = s_SurfaceDataCache.insert(mapIndex, std::move(surfaceData));
    cachePosition = insertionPosition;
    EN_ASSERT(insertionSuccess, "HeightMap insertion failed!");
  }

  return cachePosition->second;
}



static bool isEmpty(const Array3D<Block::Type, Chunk::Size()>& composition)
{
  EN_ASSERT(composition, "Composition does not exist!");
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
        if (composition[i][j][k] != Block::Type::Air)
          return false;
  return true;
}

static void heightMapStage(Array2D<length_t, Chunk::Size()>& heightMap, const GlobalIndex& chunkIndex)
{
  const auto& [noiseSamples, biomeMap] = getSurfaceData(chunkIndex);

  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      length_t elevation = 0.0;
      for (int n = 0; n < c_MaxCompoundBiomes; ++n)
      {
        const auto& [biomeType, biomeWeight] = biomeMap[i][j][n];
        if (biomeType == Biome::Type::Null)
          break;

        const Biome* biome = Biome::Get(biomeType);
        elevation += biome->localSurfaceElevation(noiseSamples[i][j]) * biomeWeight;
      }
      heightMap[i][j] = elevation;
    }
}

static void soilStage(Array3D<Block::Type, Chunk::Size()>& composition, const Array2D<length_t, Chunk::Size()>& heightMap, const GlobalIndex& chunkIndex)
{
  const auto& [noiseSamples, biomeMap] = getSurfaceData(chunkIndex);

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      const length_t& elevation = heightMap[i][j];
      const Biome* primaryBiome = Biome::Get(biomeMap[i][j][0].type);
      primaryBiome->fillColumn(composition[i][j], chunkFloor, elevation);
    }
}

static void foliageStage(Array3D<Block::Type, Chunk::Size()>& composition, const Array2D<length_t, Chunk::Size()>& heightMap, const GlobalIndex& chunkIndex)
{
  const auto createTree = [](Array3D<Block::Type, Chunk::Size()>& composition, const BlockIndex& treeIndex, Block::Type leafType)
  {
    int i = treeIndex.i;
    int j = treeIndex.j;
    int k = treeIndex.k;

    for (int n = 0; n < 5; ++n)
      composition[i][j][k + n] = Block::Type::OakLog;

    for (int I = -3; I < 3; ++I)
      for (int J = -3; J < 3; ++J)
        for (int K = 0; K < 3; ++K)
          if (I * I + J * J + K * K < 9)
            composition[i + I][j + J][k + K + 5] = leafType;
  };

  const auto& [noiseSamples, biomeMap] = getSurfaceData(chunkIndex);

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  if (chunkFloor < 0.0)
    return;

  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      Biome::Type primaryBiome = biomeMap[i][j].getPrimary();
      if (primaryBiome == Biome::Type::SuperFlat || primaryBiome == Biome::Type::GrassField)
      {
        const length_t& elevation = heightMap[i][j];
        length_t heightInChunk = elevation - chunkFloor;
        if (0 < heightInChunk && heightInChunk < Chunk::Length() - 8 * Block::Length() && 3 < i && i < Chunk::Size() - 4 && 3 < j && j < Chunk::Size() - 4)
        {
          int random = rand();
          if (random % 101 == 0)
          {
            Block::Type leafType = random % 2 == 0 ? Block::Type::OakLeaves : Block::Type::FallLeaves;
            blockIndex_t k = static_cast<blockIndex_t>(std::ceil(heightInChunk / Block::Length()));
            createTree(composition, { i, j, k }, leafType);
          }
        }
      }
    }
}

Array3D<Block::Type, Chunk::Size()> Terrain::GenerateNew(const GlobalIndex& chunkIndex)
{
  Array2D<length_t, Chunk::Size()> heightMap = AllocateArray2D<length_t, Chunk::Size()>();
  Array3D<Block::Type, Chunk::Size()> composition = AllocateArray3D<Block::Type, Chunk::Size()>();

  std::lock_guard lock(s_Mutex);

  heightMapStage(heightMap, chunkIndex);
  soilStage(composition, heightMap, chunkIndex);
  foliageStage(composition, heightMap, chunkIndex);

  if (isEmpty(composition))
    composition.reset();

  return composition;
}

Array3D<Block::Type, Chunk::Size()> Terrain::GenerateEmpty(const GlobalIndex& chunkIndex)
{
  return AllocateArray3D<Block::Type, Chunk::Size()>(Block::Type::Air);
}