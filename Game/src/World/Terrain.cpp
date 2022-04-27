#include "GMpch.h"
#include "Terrain.h"
#include "Util/Util.h"
#include "Util/Noise.h"
#include "Player/Player.h"

static constexpr Biome s_DefaultBiome{};

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
        surfaceData[i][j] = Terrain::CreateSurfaceData(blockXY, s_DefaultBiome);
      }
  }

  globalIndex_t chunkI;
  globalIndex_t chunkJ;
  HeapArray2D<Terrain::SurfaceData, Chunk::Size()> surfaceData;
};

static std::unordered_map<int, HeightMap> s_HeightMapCache;

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

static Terrain::SurfaceData getSurfaceData(const  Terrain::SurfaceData* surfaceHeights, blockIndex_t i, blockIndex_t j)
{
  EN_ASSERT(surfaceHeights, "Surface data does not exist!");
  EN_ASSERT(-Chunk::Size() <= i && i < 3 * Chunk::Size() && -Chunk::Size() <= j && j < 3 * Chunk::Size(), "Index is out of bounds!");
  return surfaceHeights[(3 * Chunk::Size()) * (i + Chunk::Size()) + (j + Chunk::Size())];
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
  int heightMapKey = Util::CreateHeightMapKey(chunkIndex.i, chunkIndex.j);
  auto it = s_HeightMapCache.find(heightMapKey);
  if (it == s_HeightMapCache.end())
  {
    const auto& [insertionPosition, insertionSuccess] = s_HeightMapCache.emplace(heightMapKey, chunkIndex);
    it = insertionPosition;
    EN_ASSERT(insertionSuccess, "HeightMap insertion failed!");
  }

  return it->second;
}



static void heightMapStage(Block::Type* composition, const GlobalIndex& chunkIndex)
{
  // Generates heightmap is none exists
  const HeightMap& heightMap = getHeightMap(chunkIndex);

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      const Terrain::SurfaceData& surfaceData = heightMap.surfaceData[i][j];
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

static void heightMapStageExperimental(Block::Type* composition, const GlobalIndex& chunkIndex)
{
  // NOTE: Should probably replace with custom memory allocation system
  static constexpr int totalAreaNeeded = (3 * Chunk::Size()) * (3 * Chunk::Size());
  static Terrain::SurfaceData* const surfaceHeights = new Terrain::SurfaceData[totalAreaNeeded];

  for (int I = 0; I < 3; ++I)
    for (int J = 0; J < 3; ++J)
    {
      const HeightMap& heightMap = getHeightMap(chunkIndex + GlobalIndex(I - 1, J - 1, 0));

      for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
        for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
          surfaceHeights[3 * Chunk::Size() * (Chunk::Size() * I + i) + Chunk::Size() * J + j] = heightMap.surfaceData[i][j];
    }

  length_t chunkFloor = Chunk::Length() * chunkIndex.k;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
      {
        length_t blockHeight = chunkFloor + Block::Length() * k;

        int I = i;
        int J = blockHeight > 100 * Block::Length() ? j + static_cast<int>((blockHeight - 100 * Block::Length()) / (2 * Block::Length())) : j;
        J = J > j + Chunk::Size() ? j + Chunk::Size() - 1 : J;

        const Terrain::SurfaceData& surfaceData = getSurfaceData(surfaceHeights, I, J);
        const Block::Type& surfaceBlockType = surfaceData.blockType;
        int terrainHeightIndex = static_cast<int>(std::floor((surfaceData.elevation - chunkFloor) / Block::Length()));

        if (k < terrainHeightIndex)
          setBlockType(composition, i, j, k, Block::Type::Dirt);
        else if (k == terrainHeightIndex)
          setBlockType(composition, i, j, k, surfaceBlockType);
        else
          setBlockType(composition, i, j, k, Block::Type::Air);
      }
}

void Terrain::GenerateNew(Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  static constexpr length_t globalMinTerrainHeight = s_DefaultBiome.minElevation();
  static constexpr length_t globalMaxTerrainHeight = s_DefaultBiome.maxElevation();

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
  // Destroy heightmaps outside of unload range
  for (auto it = s_HeightMapCache.begin(); it != s_HeightMapCache.end();)
  {
    const HeightMap& heightMap = it->second;

    if (!Util::IsInRangeOfPlayer(GlobalIndex(heightMap.chunkI, heightMap.chunkJ, Player::OriginIndex().k), unloadDistance))
      it = s_HeightMapCache.erase(it);
    else
      ++it;
  }
}

Terrain::SurfaceData Terrain::CreateSurfaceData(const Vec2& pointXY, const Biome& biome)
{
  std::array<length_t, 4> noiseOctaves = Noise::OctaveNoise2D(pointXY, biome.elevationAmplitude,
                                                                       biome.elevationScale,
                                                                       biome.elevationPersistence,
                                                                       biome.elevationLacunarity);

  length_t elevation = s_DefaultBiome.averageElevation + noiseOctaves[0] + noiseOctaves[1] + noiseOctaves[2] + noiseOctaves[3];
  Block::Type blockType = biome.primarySurfaceType.getPrimary();

  // NOTE: Should be using some sort of temperature system here
  if (elevation > 50 * Block::Length() + noiseOctaves[1])
    blockType = Block::Type::Snow;
  else if (elevation > 30 * Block::Length() + noiseOctaves[2])
    blockType = Block::Type::Stone;

  return { elevation, blockType };
}