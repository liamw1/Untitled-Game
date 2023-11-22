#pragma once
#include "Chunk.h"
#include "ChunkContainer.h"
#include "ChunkHelpers.h"
#include "LOD.h"

class ChunkManager
{
  std::shared_ptr<eng::thread::AsyncMultiDrawArray<ChunkDrawCommand>> m_OpaqueMultiDrawArray;
  std::shared_ptr<eng::thread::AsyncMultiDrawArray<ChunkDrawCommand>> m_TransparentMultiDrawArray;

  // Multi-threading
  std::shared_ptr<eng::thread::ThreadPool> m_ThreadPool;
  eng::thread::WorkSet<GlobalIndex, std::shared_ptr<Chunk>> m_LoadWork;
  eng::thread::WorkSet<GlobalIndex, void> m_CleanWork;
  eng::thread::WorkSet<GlobalIndex, void> m_LightingWork;
  eng::thread::WorkSet<GlobalIndex, void> m_LazyMeshingWork;
  eng::thread::WorkSet<GlobalIndex, void> m_ForceMeshingWork;

  // Chunk data
  ChunkContainer m_ChunkContainer;

  // LOD data
  lod::Octree m_LODTree;

public:
  ChunkManager();
  ~ChunkManager();

  void initialize();

  /*
    Decides which chunks to submit for rendering.
  */
  void render();

  /*
    Updates chunks that have been qeued for immediate updating.
  */
  void update();

  void loadNewChunks();

  /*
    Unloads boundary chunks that are out of unload range.
  */
  void clean();

  std::shared_ptr<const Chunk> getChunk(const LocalIndex& chunkIndex) const;

  void placeBlock(GlobalIndex chunkIndex, BlockIndex blockIndex, eng::math::Direction face, block::Type blockType);
  void removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  void initializeLODs();
  void renderLODs();
  void manageLODs();

private:
  void addToLightingUpdateQueue(const GlobalIndex& chunkIndex);
  void addToLazyMeshUpdateQueue(const GlobalIndex& chunkIndex);
  void addToForceMeshUpdateQueue(const GlobalIndex& chunkIndex);
  void removeMeshes(const GlobalIndex& chunkIndex);

  std::shared_ptr<Chunk> generateNewChunk(const GlobalIndex& chunkIndex);
  void eraseChunk(const GlobalIndex& chunkIndex);

  /*
    Queues chunk where the block update occured for updating. If specified block is on chunk border,
    will also update neighboring chunks. Chunk and its face neighbors are queue for an immediate update,
    while edge and corner neighbors are queued for later.
  */
  void sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  /*
    Generates simplistic mesh in a compressed format based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.
    Uses AO algorithm outlined in https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
  */
  void meshChunk(const Chunk& chunk);

  void updateLighting(Chunk& chunk);

  void lightingPacket(const GlobalIndex& chunkIndex);
  void lazyMeshingPacket(const GlobalIndex& chunkIndex);
  void forceMeshingPacket(const GlobalIndex& chunkIndex);

  bool splitLODs(std::vector<lod::Octree::Node*>& leaves);
  bool combineLODs(std::vector<lod::Octree::Node*>& leaves);
  bool splitAndCombineLODs(std::vector<lod::Octree::Node*>& leaves);
};