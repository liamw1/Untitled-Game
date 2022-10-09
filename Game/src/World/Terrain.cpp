#include "GMpch.h"
#include "Terrain.h"
#include "Util/Util.h"
#include "Util/Noise.h"
#include "Player/Player.h"

static const Biome s_DefaultBiome = Biome::Get(Biome::Type::Default);

class ChunkFiller
{
public:
  static void Clear(Chunk* chunk) { chunk->clear(); }
  static void SetData(Chunk* chunk, Block::Type* composition) { chunk->setData(composition); }
};

struct HeightMap
{
  HeightMap(const GlobalIndex& index)
    : chunkI(index.i), chunkJ(index.j)
  {
    EN_PROFILE_FUNCTION();

    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        Vec2 blockXY = Chunk::Length() * Vec2(chunkI, chunkJ) + Block::Length() * (Vec2(i, j) + Vec2(0.5));
        elevationData[i][j] = Terrain::GetElevationData(blockXY, s_DefaultBiome);
      }
  }

  globalIndex_t chunkI;
  globalIndex_t chunkJ;
  HeapArray2D<Noise::OctaveNoiseData<Biome::NumOctaves()>, Chunk::Size()> elevationData;
};

struct TemperatureMap
{
  TemperatureMap(const GlobalIndex& index)
    : chunkI(index.i), chunkJ(index.j)
  {
    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        Vec2 blockXY = Chunk::Length() * Vec2(chunkI, chunkJ) + Block::Length() * (Vec2(i, j) + Vec2(0.5));
        temperatureData[i][j] = Terrain::GetTemperatureData(blockXY, s_DefaultBiome);
      }
  }

  globalIndex_t chunkI;
  globalIndex_t chunkJ;
  StackArray2D<float, Chunk::Size()> temperatureData{};
};

struct BiomeMap
{
  BiomeMap(const GlobalIndex& index)
    : chunkI(index.i), chunkJ(index.j)
  {
    // Generate Voronoi points
    std::array<Vec2, 9> voronoiPoints{};

    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        Vec2 blockXY = Chunk::Length() * Vec2(chunkI, chunkJ) + Block::Length() * (Vec2(i, j) + Vec2(0.5));


      }
  }

  globalIndex_t chunkI;
  globalIndex_t chunkJ;
  StackArray2D<Biome, Chunk::Size()> biomeData{};
};

static std::unordered_map<int, HeightMap> s_HeightMapCache;
static std::unordered_map<int, TemperatureMap> s_TemperatureMapCache;
static std::mutex s_Mutex;

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

  textureIndices[0] = static_cast<int>(Block::GetTexture(m_Components.getType(0), Block::Face::Top));
  textureIndices[1] = static_cast<int>(Block::GetTexture(m_Components.getType(1), Block::Face::Top));

  return textureIndices;
}



static Block::Type getBlockType(const Block::Type* composition, blockIndex_t i, blockIndex_t j, blockIndex_t k)
{
  EN_ASSERT(composition, "Composition does not exist!");
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");
  return composition[Chunk::Size() * Chunk::Size() * i + Chunk::Size() * j + k];
}

static void setBlockType(Block::Type* composition, blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  EN_ASSERT(composition, "Composition does not exist!");
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");
  composition[Chunk::Size() * Chunk::Size() * i + Chunk::Size() * j + k] = blockType;
}

static bool isEmpty(Block::Type* composition)
{
  EN_ASSERT(composition, "Composition does not exist!");
  for (int i = 0; i < Chunk::TotalBlocks(); ++i)
    if (composition[i] != Block::Type::Air)
      return false;
  return true;
}

static const HeightMap& getHeightMap(const GlobalIndex& chunkIndex)
{
  int heightMapKey = Util::Create2DMapKey(chunkIndex.i, chunkIndex.j);
  auto it = s_HeightMapCache.find(heightMapKey);
  if (it == s_HeightMapCache.end())
  {
    const auto& [insertionPosition, insertionSuccess] = s_HeightMapCache.emplace(heightMapKey, chunkIndex);
    it = insertionPosition;
    EN_ASSERT(insertionSuccess, "HeightMap insertion failed!");
  }

  return it->second;
}

static const TemperatureMap& getTemperatureMap(const GlobalIndex& chunkIndex)
{
  int temperatureMapKey = Util::Create2DMapKey(chunkIndex.i, chunkIndex.j);
  auto it = s_TemperatureMapCache.find(temperatureMapKey);
  if (it == s_TemperatureMapCache.end())
  {
    const auto& [insertionPosition, insertionSuccess] = s_TemperatureMapCache.emplace(temperatureMapKey, chunkIndex);
    it = insertionPosition;
    EN_ASSERT(insertionSuccess, "TemperatureMap insertion failed!");
  }

  return it->second;
}



static float calcSurfaceTemperature(float seaLevelTemperature, length_t surfaceElevation)
{
  static constexpr float tempDropPerBlock = 0.25f;
  return seaLevelTemperature - tempDropPerBlock * static_cast<float>(surfaceElevation / Block::Length());
}

static void heightMapStage(Block::Type* composition, const GlobalIndex& chunkIndex)
{
  // Generates heightmap is none exists
  const HeightMap& heightMap = getHeightMap(chunkIndex);
  const TemperatureMap& temperatureMap = getTemperatureMap(chunkIndex);

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      Terrain::SurfaceData surfaceData = Terrain::GetSurfaceData(heightMap.elevationData[i][j], temperatureMap.temperatureData[i][j], s_DefaultBiome);

      int terrainElevationIndex = static_cast<int>(std::ceil((surfaceData.elevation - chunkFloor) / Block::Length()));
      int surfaceDepth = static_cast<int>(std::ceil(s_DefaultBiome.averageSurfaceDepth));
      int soilDepth = surfaceDepth + static_cast<int>(std::ceil(s_DefaultBiome.averageSoilDepth));

      blockIndex_t k = 0;
      while (k < terrainElevationIndex - soilDepth && k < Chunk::Size())
      {
        setBlockType(composition, i, j, k, Block::Type::Stone);
        k++;
      }
      while (k < terrainElevationIndex - surfaceDepth && k < Chunk::Size())
      {
        setBlockType(composition, i, j, k, s_DefaultBiome.soilType.getPrimary());
        k++;
      }
      while (k < terrainElevationIndex && k < Chunk::Size())
      {
        setBlockType(composition, i, j, k, surfaceData.blockType);
        k++;
      }
      while (k < Chunk::Size())
      {
        setBlockType(composition, i, j, k, Block::Type::Air);
        k++;
      }
    }
}

void Terrain::GenerateNew(Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  std::lock_guard lock(s_Mutex);

  static const length_t globalMinTerrainHeight = s_DefaultBiome.minElevation();
  static const length_t globalMaxTerrainHeight = s_DefaultBiome.maxElevation();

  if (!chunk->isEmpty())
  {
    EN_WARN("Calling generate on a non-empty chunk!  Deleting previous allocation...");
    ChunkFiller::Clear(chunk);
  }

  length_t chunkFloor = Chunk::Length() * chunk->getGlobalIndex().k;
  if (chunkFloor > globalMaxTerrainHeight)
  {
    ChunkFiller::SetData(chunk, nullptr);
    return;
  }

  Block::Type* composition = new Block::Type[Chunk::TotalBlocks()];
  heightMapStage(composition, chunk->getGlobalIndex());

  if (isEmpty(composition))
  {
    delete[] composition;
    composition = nullptr;
  }

  // Chunk takes ownership of composition
  ChunkFiller::SetData(chunk, composition);
}

void Terrain::Clean(int unloadDistance)
{
  std::lock_guard lock(s_Mutex);

  // Destroy heightmaps outside of unload range
  for (auto it = s_HeightMapCache.begin(); it != s_HeightMapCache.end();)
  {
    const HeightMap& heightMap = it->second;

    if (!Util::IsInRangeOfPlayer(GlobalIndex(heightMap.chunkI, heightMap.chunkJ, Player::OriginIndex().k), unloadDistance))
      it = s_HeightMapCache.erase(it);
    else
      ++it;
  }

  // Destory temperature maps outside of unload range
  for (auto it = s_TemperatureMapCache.begin(); it != s_TemperatureMapCache.end();)
  {
    const TemperatureMap& temperatureMap = it->second;

    if (!Util::IsInRangeOfPlayer(GlobalIndex(temperatureMap.chunkI, temperatureMap.chunkJ, Player::OriginIndex().k), unloadDistance))
      it = s_TemperatureMapCache.erase(it);
    else
      ++it;
  }
}

Noise::OctaveNoiseData<Biome::NumOctaves()> Terrain::GetElevationData(const Vec2& pointXY, const Biome& biome)
{
  return Noise::OctaveNoise2D<Biome::NumOctaves()>(pointXY, biome.elevationAmplitude,
                                                            biome.elevationScale,
                                                            biome.elevationPersistence,
                                                            biome.elevationLacunarity);
}

float Terrain::GetTemperatureData(const Vec2& pointXY, const Biome& biome)
{
  float localTemperatureVariation = biome.localTemperatureVariation * static_cast<float>(Noise::SimplexNoise2D(pointXY / biome.localTemperatureVariationScale));
  return biome.averageTemperature + localTemperatureVariation;
}

static Block::Type determineSurfaceBlockType(length_t surfaceElevation, float surfaceTemperature, const Biome& biome, length_t variation)
{
  if (surfaceTemperature < biome.coldThreshold)
    return biome.surfaceType_Cold.getPrimary();
  else if (surfaceElevation > biome.highThreshold + variation)
    return biome.surfaceType_High.getPrimary();
  else
    return biome.surfaceType.getPrimary();
}

Terrain::SurfaceData Terrain::GetSurfaceData(const Noise::OctaveNoiseData<Biome::NumOctaves()>& elevationData, float seaLevelTemperature, const Biome& biome)
{
  length_t elevation = elevationData.sum();
  float surfaceTemperature = calcSurfaceTemperature(seaLevelTemperature, elevation);
  Block::Type blockType = determineSurfaceBlockType(elevation, surfaceTemperature, biome, elevationData[2]);

  return { elevation, blockType };
}
