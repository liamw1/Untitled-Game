#pragma once
#include "Chunk.h"
#include "ChunkContainer.h"
#include "ChunkHelpers.h"

class ChunkManager
{
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

  void placeBlock(GlobalIndex chunkIndex, BlockIndex blockIndex, Direction face, Block::Type blockType);
  void removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  // Debug
  void loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType);

private:
  struct LightUniforms
  {
    const float maxSunlight = static_cast<float>(Block::Light::MaxValue());
    float sunIntensity = 1.0f;
  };

  // Rendering
  static inline std::unique_ptr<Engine::Shader> s_Shader;
  static inline std::unique_ptr<Engine::Uniform> s_LightUniform;
  static inline std::unique_ptr<Engine::StorageBuffer> s_SSBO;
  static inline std::shared_ptr<Engine::TextureArray> s_TextureArray;
  static inline const Engine::BufferLayout s_VertexBufferLayout = { { ShaderDataType::Uint32, "a_VertexData" },
                                                                    { ShaderDataType::Uint32, "a_Lighting"   } };
  static constexpr int c_TextureSlot = 0;
  static constexpr int c_StorageBufferBinding = 0;
  static constexpr uint32_t c_StorageBufferSize = static_cast<uint32_t>(Engine::Pow2(20));

  Engine::Threads::UnorderedSet<ChunkDrawCommand> m_OpaqueCommandQueue;
  Engine::Threads::UnorderedSet<ChunkDrawCommand> m_TransparentCommandQueue;
  std::unique_ptr<Engine::MultiDrawIndexedArray<ChunkDrawCommand>> m_OpaqueMultiDrawArray;
  std::unique_ptr<Engine::MultiDrawIndexedArray<ChunkDrawCommand>> m_TransparentMultiDrawArray;

  // Multi-threading
  ChunkContainer m_ChunkContainer;
  std::shared_ptr<Engine::Threads::ThreadPool> m_ThreadPool;
  Engine::Threads::WorkSet<GlobalIndex, std::shared_ptr<Chunk>> m_LoadWork;
  Engine::Threads::WorkSet<GlobalIndex, void> m_CleanWork;
  Engine::Threads::WorkSet<GlobalIndex, void> m_LightingWork;
  Engine::Threads::WorkSet<GlobalIndex, void> m_LazyMeshingWork;
  Engine::Threads::WorkSet<GlobalIndex, void> m_ForceMeshingWork;

  GlobalIndex m_PrevPlayerOriginIndex;

  void addToLightingUpdateQueue(const GlobalIndex& chunkIndex);
  void addToLazyMeshUpdateQueue(const GlobalIndex& chunkIndex);
  void addToForceMeshUpdateQueue(const GlobalIndex& chunkIndex);
  void addToMeshRemovalQueue(const GlobalIndex& chunkIndex);

  std::shared_ptr<Chunk> generateNewChunk(const GlobalIndex& chunkIndex);
  void eraseChunk(const GlobalIndex& chunkIndex);

  /*
    Queues chunk where the block update occured for updating. If specified block is on chunk border,
    will also update neighboring chunks. Chunk and its cardinal neighbors are queue for an immediate update,
    while edge and corner neighbors are queued for later.
  */
  void sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  void uploadMeshes(Engine::Threads::UnorderedSet<ChunkDrawCommand>& commandQueue, std::unique_ptr<Engine::MultiDrawIndexedArray<ChunkDrawCommand>>& multiDrawArray);

  // Meshing
private:
  /*
    Generates simplistic mesh in a compressed format based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.
    Compresed format is follows,
     bits 0-17:  Relative position of vertex within chunk (3-comps, 6 bits each)
     bits 18-19: Quad index
     bits 20-21: Ambient Occlusion level (see link below)
     bits 22-31: Texure ID
     Uses AO algorithm outlined in https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
  */
  void meshChunk(const std::shared_ptr<Chunk>& chunk);

  void updateLighting(const std::shared_ptr<Chunk>& chunk);

  void lightingPacket(const GlobalIndex& chunkIndex);
  void lazyMeshingPacket(const GlobalIndex& chunkIndex);
  void forceMeshingPacket(const GlobalIndex& chunkIndex);
};