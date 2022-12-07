#include "GMpch.h"
#include "Terrain.h"
#include "Util/Util.h"
#include "Util/Noise.h"
#include "Player/Player.h"

static const Biome s_DefaultBiome = Biome::Get(Biome::Type::Default);

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

  textureIndices[0] = static_cast<int>(Block::GetTexture(m_Components.getType(0), Block::Face::Top));
  textureIndices[1] = static_cast<int>(Block::GetTexture(m_Components.getType(1), Block::Face::Top));

  return textureIndices;
}



using ElevationData = HeapArray2D<Noise::OctaveNoiseData<Biome::NumOctaves()>, Chunk::Size()>;
using TemperatureData = HeapArray2D<float, Chunk::Size()>;

struct SurfaceData
{
  ElevationData elevationData;
  TemperatureData temperatureData;
};

static std::unordered_map<SurfaceMapIndex, SurfaceData> s_SurfaceDataCache;
static std::mutex s_Mutex;

static float sampleElevation(const GlobalIndex& chunkIndex)
{
  static constexpr float averageElevation = 0 * Block::LengthF();
  static constexpr float elevationScale = 64.0f;
  static constexpr float elevationAmplitude = 100 * Block::LengthF();

  Float2 samplingPosition = static_cast<Float2>(chunkIndex) / elevationScale;
  return averageElevation + elevationAmplitude * Noise::SimplexNoise2D(samplingPosition);
}

static float sampleTemperature(const GlobalIndex& chunkIndex)
{
  static constexpr float averageTemperature = 20.0f;
  static constexpr float temperatureScale = 32.0f;
  static constexpr float temperatureAmplitude = 30.0f;

  Float2 samplingPosition = static_cast<Float2>(chunkIndex) / temperatureScale;
  return averageTemperature + temperatureAmplitude * Noise::SimplexNoise2D(samplingPosition);
}

static float sampleHumidity(const GlobalIndex& chunkIndex)
{
  static constexpr float averageHumidity = 50.0f;
  static constexpr float humidityScale = 16.0f;
  static constexpr float humidityAmplitude = 50.0f;

  Float2 samplingPosition = static_cast<Float2>(chunkIndex) / humidityScale;
  return averageHumidity + humidityAmplitude * Noise::SimplexNoise2D(samplingPosition);
}

static const SurfaceData& getSurfaceData(const GlobalIndex& chunkIndex)
{
  SurfaceMapIndex mapIndex = static_cast<SurfaceMapIndex>(chunkIndex);
  auto it = s_SurfaceDataCache.find(mapIndex);
  if (it == s_SurfaceDataCache.end())
  {
    // Generate surface data
    SurfaceData surfaceData{};
    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        Vec2 blockXY = Chunk::Length() * static_cast<Vec2>(mapIndex) + Block::Length() * (Vec2(i, j) + Vec2(0.5));

        surfaceData.elevationData[i][j] = Terrain::GetElevationData(blockXY, s_DefaultBiome);
        surfaceData.temperatureData[i][j] = Terrain::GetTemperatureData(blockXY, s_DefaultBiome);
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
  // Generates surface data if none exists
  const auto& [heightMap, temperatureMap] = getSurfaceData(chunkIndex);

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      Terrain::SurfaceInfo surfaceInfo = Terrain::GetSurfaceInfo(heightMap[i][j], temperatureMap[i][j], s_DefaultBiome);

      int terrainElevationIndex = static_cast<int>(std::ceil((surfaceInfo.elevation - chunkFloor) / Block::Length()));
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
        setBlockType(composition, i, j, k, surfaceInfo.blockType);
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

  static const length_t globalMinTerrainHeight = s_DefaultBiome.minElevation();
  static const length_t globalMaxTerrainHeight = s_DefaultBiome.maxElevation();

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  if (chunkFloor > globalMaxTerrainHeight)
  {
    ChunkFiller::SetData(chunk, nullptr);
    return chunk;
  }

  std::unique_ptr<Block::Type[]> composition = std::make_unique_for_overwrite<Block::Type[]>(Chunk::TotalBlocks());
  std::lock_guard lock(s_Mutex);

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

Terrain::SurfaceInfo Terrain::GetSurfaceInfo(const Noise::OctaveNoiseData<Biome::NumOctaves()>& elevationData, float seaLevelTemperature, const Biome& biome)
{
  length_t elevation = elevationData.sum();
  float surfaceTemperature = calcSurfaceTemperature(seaLevelTemperature, elevation);
  Block::Type blockType = determineSurfaceBlockType(elevation, surfaceTemperature, biome, elevationData[2]);

  return { elevation, blockType };
}
