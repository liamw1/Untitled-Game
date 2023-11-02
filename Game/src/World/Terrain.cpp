#include "GMpch.h"
#include "Terrain.h"
#include "ChunkContainer.h"
#include "Util/Noise.h"
#include "Util/Util.h"
#include "Player/Player.h"

terrain::CompoundSurfaceData::CompoundSurfaceData() = default;
terrain::CompoundSurfaceData::CompoundSurfaceData(length_t surfaceElevation, block::ID blockType)
  : m_Elevation(surfaceElevation), m_Components(blockType) {}

terrain::CompoundSurfaceData terrain::CompoundSurfaceData::operator+(const CompoundSurfaceData& other) const
{
  CompoundSurfaceData sum = *this;
  sum.m_Elevation += other.m_Elevation;
  sum.m_Components += other.m_Components;

  return sum;
}

terrain::CompoundSurfaceData terrain::CompoundSurfaceData::operator*(f32 x) const
{
  CompoundSurfaceData result = *this;
  result.m_Elevation *= x;
  result.m_Components *= x;

  return result;
}

length_t terrain::CompoundSurfaceData::getElevation() const
{
  return m_Elevation;
}

block::Type terrain::CompoundSurfaceData::getPrimaryBlockType() const
{
  return m_Components.getPrimary();
}

std::array<i32, 2> terrain::CompoundSurfaceData::getTextureIndices() const
{
  std::array<i32, 2> textureIndices{};

  textureIndices[0] = static_cast<i32>(m_Components[0].type.texture(eng::math::Direction::Top));
  textureIndices[1] = static_cast<i32>(m_Components[1].type.texture(eng::math::Direction::Top));

  return textureIndices;
}

eng::math::Float2 terrain::CompoundSurfaceData::getTextureWeights() const
{
  return { m_Components[0].weight, m_Components[1].weight };
}



static constexpr length_t c_LargestNoiseScale = 1024 * block::length();
static constexpr f32 c_NoiseLacunarity = 2.0f;

static constexpr i32 c_MaxCompoundBiomes = 4;
static constexpr i32 c_BiomeRegionSize = 8;
static constexpr i32 c_RegionRadius = 1;
static constexpr i32 c_RegionWidth = 2 * c_RegionRadius + 1;

using NoiseSamples = BlockArrayRect<noise::OctaveNoiseData<Biome::LocalElevationOctaves()>>;
using CompoundBiome = CompoundType<Biome::Type, c_MaxCompoundBiomes>;
using BiomeData = BlockArrayRect<CompoundBiome>;

struct SurfaceData
{
  NoiseSamples noiseSamples = BlockArrayRect<noise::OctaveNoiseData<Biome::LocalElevationOctaves()>>(Chunk::Bounds2D(), eng::AllocationPolicy::ForOverwrite);
  BiomeData biomeData = BlockArrayRect<CompoundBiome>(Chunk::Bounds2D(), eng::AllocationPolicy::ForOverwrite);
};

static constexpr i32 c_CacheSize = (2 * c_UnloadDistance + 5) * (2 * c_UnloadDistance + 5);
static eng::thread::LRUCache<GlobalIndex2D, SurfaceData> s_SurfaceDataCache(c_CacheSize);
static std::mutex s_Mutex;

// From https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
static u32 hash(u32 n)
{
  n = ((n >> 16) ^ n) * 0x45d9f3b;
  n = ((n >> 16) ^ n) * 0x45d9f3b;
  return (n >> 16) ^ n;
}

// Returns a random f32 in the range [0.0, 1.0] based determinisitically on the input n
static f32 random(u32 n)
{
  return static_cast<f32>(hash(n)) / std::numeric_limits<u32>::max();
}

// Returns a biome type based determinisitically on the input n
static Biome::Type randomBiome(u32 n)
{
  return static_cast<Biome::Type>(hash(n) % Biome::Count());
}

static std::pair<Biome::Type, eng::math::Float2> getRegionVoronoiPoint(const GlobalIndex2D& regionIndex)
{
  u32 key = static_cast<u32>(std::hash<GlobalIndex2D>{}(regionIndex));
  f32 r = 0.5f * random(key);
  f32 theta = 2 * std::numbers::pi_v<f32> * random(hash(key));

  Biome::Type biomeType = randomBiome(key);
  eng::math::Float2 relativeLocation(r * std::cos(theta), r * std::sin(theta));
  return { biomeType, relativeLocation };
}

static CompoundBiome getBiomeData(const eng::math::Vec2& surfaceLocation)
{
  using WeightedBiome = CompoundBiome::Component;

  GlobalIndex2D queryRegionIndex = GlobalIndex2D::ToIndex(surfaceLocation / Chunk::Size() / c_BiomeRegionSize);
  eng::math::Float2 queryLocationRelativeToQueryRegion = surfaceLocation / Chunk::Size() / c_BiomeRegionSize - static_cast<eng::math::Vec2>(queryRegionIndex);

  std::array<WeightedBiome, c_RegionWidth* c_RegionWidth> nearbyBiomes;
  for (globalIndex_t i = -c_RegionRadius; i <= c_RegionRadius; ++i)
    for (globalIndex_t j = -c_RegionRadius; j <= c_RegionRadius; ++j)
    {
      GlobalIndex2D regionIndex = queryRegionIndex + GlobalIndex2D(i, j);
      eng::math::Float2 regionCenterRelativeToQueryRegion(i + 0.5f, j + 0.5f);

      auto [biomeType, voronoiPointPerturbation] = getRegionVoronoiPoint(regionIndex);
      eng::math::Float2 voronoiPointRelativeToQueryRegion = regionCenterRelativeToQueryRegion + voronoiPointPerturbation;
      f32 distance = glm::distance(queryLocationRelativeToQueryRegion, voronoiPointRelativeToQueryRegion);
      f32 biomeWeight = std::expf(-32 * distance * distance);

      i32 index = c_RegionWidth * (i + c_RegionRadius) + j + c_RegionRadius;
      nearbyBiomes[index] = { biomeType, biomeWeight };
    }

  // Combine components of same type
  eng::algo::sort(nearbyBiomes, [](const WeightedBiome& biome) { return eng::toUnderlying(biome.type); }, eng::SortPolicy::Ascending);
  i32 lastUniqueElementIndex = 0;
  for (i32 i = 1; i < nearbyBiomes.size(); ++i)
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
  eng::algo::sort(nearbyBiomes, [](const WeightedBiome& biome) { return biome.weight; }, eng::SortPolicy::Descending);
  return CompoundBiome(nearbyBiomes);
}

static std::shared_ptr<SurfaceData> getSurfaceData(const GlobalIndex& chunkIndex)
{
  GlobalIndex2D mapIndex = static_cast<GlobalIndex2D>(chunkIndex);
  std::shared_ptr<SurfaceData> cachedSurfaceData = s_SurfaceDataCache.get(mapIndex);
  if (cachedSurfaceData)
    return cachedSurfaceData;

  // Generate surface data
  std::shared_ptr<SurfaceData> surfaceData = std::make_shared<SurfaceData>();
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      eng::math::Vec2 blockXY = Chunk::Length() * static_cast<eng::math::Vec2>(mapIndex) + block::length() * (eng::math::Vec2(i, j) + eng::math::Vec2(0.5));

      surfaceData->noiseSamples[i][j] = noise::octaveNoise2D<Biome::LocalElevationOctaves()>(blockXY, 1_m / c_LargestNoiseScale, c_NoiseLacunarity);
      surfaceData->biomeData[i][j] = getBiomeData(blockXY);
    }

  s_SurfaceDataCache.insert(mapIndex, surfaceData);
  return surfaceData;
}



static void heightMapStage(BlockArrayRect<length_t>& heightMap, const GlobalIndex& chunkIndex)
{
  std::shared_ptr<SurfaceData> surfaceData = getSurfaceData(chunkIndex);
  const auto& [noiseSamples, biomeMap] = *surfaceData;

  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      length_t elevation = 0.0;
      for (i32 n = 0; n < c_MaxCompoundBiomes; ++n)
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

static void soilStage(BlockArrayBox<block::Type>& composition, const BlockArrayRect<length_t>& heightMap, const GlobalIndex& chunkIndex)
{
  std::shared_ptr<SurfaceData> surfaceData = getSurfaceData(chunkIndex);
  const auto& [noiseSamples, biomeMap] = *surfaceData;

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      const length_t& elevation = heightMap[i][j];
      const Biome* primaryBiome = Biome::Get(biomeMap[i][j][0].type);
      primaryBiome->fillColumn(composition[i][j], chunkFloor, elevation);
    }
}

static void foliageStage(BlockArrayBox<block::Type>& composition, const BlockArrayRect<length_t>& heightMap, const GlobalIndex& chunkIndex)
{
  const auto createTree = [](BlockArrayBox<block::Type>& composition, const BlockIndex& treeIndex, block::Type leafType)
  {
    i32 i = treeIndex.i;
    i32 j = treeIndex.j;
    i32 k = treeIndex.k;

    for (i32 n = 0; n < 5; ++n)
      composition[i][j][k + n] = block::ID::OakLog;

    for (i32 I = -3; I < 3; ++I)
      for (i32 J = -3; J < 3; ++J)
        for (i32 K = 0; K < 3; ++K)
          if (I * I + J * J + K * K < 9)
            composition[i + I][j + J][k + K + 5] = leafType;
  };

  std::shared_ptr<SurfaceData> surfaceData = getSurfaceData(chunkIndex);
  const auto& [noiseSamples, biomeMap] = *surfaceData;

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
        if (0 < heightInChunk && heightInChunk < Chunk::Length() - 8 * block::length() && 3 < i && i < Chunk::Size() - 4 && 3 < j && j < Chunk::Size() - 4)
        {
          i32 random = rand();
          if (random % 101 == 0)
          {
            block::ID leafType = random % 2 == 0 ? block::ID::OakLeaves : block::ID::FallLeaves;
            blockIndex_t k = static_cast<blockIndex_t>(std::ceil(heightInChunk / block::length()));
            createTree(composition, { i, j, k }, leafType);
          }
        }
      }
    }
}

static void lightingStage(BlockArrayBox<block::Light>& lighting, const BlockArrayBox<block::Type>& composition)
{
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      blockIndex_t k = 0;
      while (k < Chunk::Size() && !composition[i][j][k].hasTransparency())
      {
        lighting[i][j][k] = block::Light(0);
        k++;
      }
      for (; k < Chunk::Size(); ++k)
        lighting[i][j][k] = block::Light(block::Light::MaxValue());
    }
}

std::shared_ptr<Chunk> terrain::generateNew(const GlobalIndex& chunkIndex)
{
  BlockArrayRect<length_t> heightMap(Chunk::Bounds2D(), eng::AllocationPolicy::ForOverwrite);
  BlockArrayBox<block::Type> composition(Chunk::Bounds(), eng::AllocationPolicy::ForOverwrite);

  heightMapStage(heightMap, chunkIndex);
  soilStage(composition, heightMap, chunkIndex);
  foliageStage(composition, heightMap, chunkIndex);

  if (composition.filledWith(block::ID::Air))
    composition.clear();

  std::shared_ptr newChunk = std::make_shared<Chunk>(chunkIndex);
  if (composition)
  {
    BlockArrayBox<block::Light> lighting(Chunk::Bounds(), eng::AllocationPolicy::ForOverwrite);
    lightingStage(lighting, composition);

    newChunk->setComposition(std::move(composition));
    newChunk->setLighting(std::move(lighting));
  }

  return newChunk;
}

std::shared_ptr<Chunk> terrain::generateEmpty(const GlobalIndex& chunkIndex)
{
  return std::make_shared<Chunk>(chunkIndex);
}
