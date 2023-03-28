#include "GMpch.h"
#include "Terrain.h"
#include "Util/Util.h"
#include "Util/Noise.h"
#include "Player/Player.h"

class ChunkFiller
{
public:
  static void SetData(Chunk& chunk, std::unique_ptr<Block::Type[]> composition) { chunk.setData(std::move(composition)); }
};

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

  textureIndices[0] = static_cast<int>(Block::GetTexture(m_Components[0].type, Block::Face::Top));
  textureIndices[1] = static_cast<int>(Block::GetTexture(m_Components[1].type, Block::Face::Top));

  return textureIndices;
}



static constexpr length_t c_LargestNoiseScale = 1024 * Block::Length();
static constexpr float c_NoiseLacunarity = 2.0f;

static constexpr int c_MaxCompoundBiomes = 4;
static constexpr int c_BiomeRegionSize = 4;
static constexpr int c_RegionRadius = 1;
static constexpr int c_RegionWidth = 2 * c_RegionRadius + 1;

using NoiseSamples = HeapArray2D<Noise::OctaveNoiseData<Biome::LocalElevationOctaves()>, Chunk::Size()>;
using BiomeData = HeapArray2D<CompoundType<Biome::Type, c_MaxCompoundBiomes>, Chunk::Size()>;

struct SurfaceData
{
  NoiseSamples noiseSamples;
  BiomeData biomeData;
};

static std::unordered_map<SurfaceMapIndex, SurfaceData> s_SurfaceDataCache;
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

static CompoundType<Biome::Type, c_MaxCompoundBiomes> getBiomeData(const Vec2& surfaceLocation)
{
  using WeightedBiome = CompoundType<Biome::Type, c_MaxCompoundBiomes>::Component;

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

  std::sort(nearbyBiomes.begin(), nearbyBiomes.end(), [](const auto& biomeA, const auto& biomeB) { return biomeA.weight > biomeB.weight; });
  return CompoundType<Biome::Type, c_MaxCompoundBiomes>(nearbyBiomes);
}

static const SurfaceData& getSurfaceData(const GlobalIndex& chunkIndex)
{
  std::lock_guard lock(s_Mutex);

  SurfaceMapIndex mapIndex = static_cast<SurfaceMapIndex>(chunkIndex);
  auto it = s_SurfaceDataCache.find(mapIndex);
  if (it == s_SurfaceDataCache.end())
  {
    EN_PROFILE_FUNCTION();
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

    const auto& [insertionPosition, insertionSuccess] = s_SurfaceDataCache.insert({ mapIndex, std::move(surfaceData) });
    it = insertionPosition;
    EN_ASSERT(insertionSuccess, "HeightMap insertion failed!");
  }

  return it->second;
}



static float calcSurfaceTemperature(float seaLevelTemperature, length_t surfaceElevation)
{
  static constexpr float tempDropPerBlock = 0.25f;
  return seaLevelTemperature - tempDropPerBlock * static_cast<float>(surfaceElevation / Block::Length());
}

static Block::Type getBlockType(const std::unique_ptr<Block::Type[]>& composition, blockIndex_t i, blockIndex_t j, blockIndex_t k)
{
  EN_ASSERT(composition, "Composition does not exist!");
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");
  return composition[Chunk::Size() * Chunk::Size() * i + Chunk::Size() * j + k];
}

static void setBlockType(std::unique_ptr<Block::Type[]>& composition, blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  EN_ASSERT(composition, "Composition does not exist!");
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");
  composition[Chunk::Size() * Chunk::Size() * i + Chunk::Size() * j + k] = blockType;
}

static bool isEmpty(const std::unique_ptr<Block::Type[]>& composition)
{
  EN_ASSERT(composition, "Composition does not exist!");
  for (int i = 0; i < Chunk::TotalBlocks(); ++i)
    if (composition[i] != Block::Type::Air)
      return false;
  return true;
}

static void heightMapStage(std::unique_ptr<Block::Type[]>& composition, const GlobalIndex& chunkIndex)
{
  EN_PROFILE_FUNCTION();

  // Generates surface data if none exists
  const auto& [noiseSamples, biomeMap] = getSurfaceData(chunkIndex);

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      length_t elevation = 0.0;
      for (int n = 0; n < c_MaxCompoundBiomes; ++n)
      {
        const Biome* biome = Biome::Get(biomeMap[i][j][n].type);
        elevation += biome->localSurfaceElevation(noiseSamples[i][j]) * biomeMap[i][j][n].weight;
      }
      const Biome* primaryBiome = Biome::Get(biomeMap[i][j][0].type);

      int surfaceDepth = 1;
      int soilDepth = 5;
      Block::Type surfaceType = primaryBiome->primarySurfaceType();
      Block::Type soilType = Block::Type::Dirt;

      int terrainElevationIndex = static_cast<int>(std::ceil((elevation - chunkFloor) / Block::Length()));

      blockIndex_t k = 0;
      while (k < terrainElevationIndex - soilDepth && k < Chunk::Size())
      {
        setBlockType(composition, i, j, k, Block::Type::Stone);
        k++;
      }
      while (k < terrainElevationIndex - surfaceDepth && k < Chunk::Size())
      {
        setBlockType(composition, i, j, k, soilType);
        k++;
      }
      while (k < terrainElevationIndex && k < Chunk::Size())
      {
        setBlockType(composition, i, j, k, surfaceType);
        k++;
      }
      while (k < Chunk::Size())
      {
        setBlockType(composition, i, j, k, Block::Type::Air);
        k++;
      }
    }
}

Chunk Terrain::GenerateNew(const GlobalIndex& chunkIndex)
{
  Chunk chunk(chunkIndex);

  std::unique_ptr<Block::Type[]> composition = std::make_unique_for_overwrite<Block::Type[]>(Chunk::TotalBlocks());
  heightMapStage(composition, chunkIndex);

  if (isEmpty(composition))
    composition.reset();

  // Chunk takes ownership of composition
  ChunkFiller::SetData(chunk, std::move(composition));
  return chunk;
}

Chunk Terrain::GenerateEmpty(const GlobalIndex& chunkIndex)
{
  Chunk chunk(chunkIndex);
  ChunkFiller::SetData(chunk, nullptr);
  return chunk;
}

void Terrain::Clean(int unloadDistance)
{
  std::lock_guard lock(s_Mutex);

  // Destroy surface maps outside of unload range
  for (auto it = s_SurfaceDataCache.begin(); it != s_SurfaceDataCache.end();)
  {
    const auto& [index, heightMap] = *it;

    if (!Util::IsInRangeOfPlayer(index, unloadDistance))
      it = s_SurfaceDataCache.erase(it);
    else
      ++it;
  }
}
