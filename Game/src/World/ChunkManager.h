#pragma once
#include "Chunk.h"
#include "ChunkContainer.h"
#include "ChunkHelpers.h"

class ChunkManager
{
  // Rendering
  static inline std::unique_ptr<eng::Shader> s_Shader;
  static inline std::unique_ptr<eng::Uniform> s_LightUniform;
  static inline std::unique_ptr<eng::StorageBuffer> s_SSBO;
  static inline std::shared_ptr<eng::TextureArray> s_TextureArray;
  static inline const eng::BufferLayout s_VertexBufferLayout = {{ eng::ShaderDataType::Uint32, "a_VertexData" },
                                                                { eng::ShaderDataType::Uint32, "a_Lighting"   }};
  static constexpr i32 c_TextureSlot = 0;
  static constexpr i32 c_StorageBufferBinding = 0;
  static constexpr u32 c_StorageBufferSize = static_cast<u32>(eng::pow2(20));

  eng::threads::UnorderedSet<ChunkDrawCommand> m_OpaqueCommandQueue;
  eng::threads::UnorderedSet<ChunkDrawCommand> m_TransparentCommandQueue;
  std::unique_ptr<eng::MultiDrawIndexedArray<ChunkDrawCommand>> m_OpaqueMultiDrawArray;
  std::unique_ptr<eng::MultiDrawIndexedArray<ChunkDrawCommand>> m_TransparentMultiDrawArray;

  // Multi-threading
  ChunkContainer m_ChunkContainer;
  std::shared_ptr<eng::threads::ThreadPool> m_ThreadPool;
  eng::threads::WorkSet<GlobalIndex, std::shared_ptr<Chunk>> m_LoadWork;
  eng::threads::WorkSet<GlobalIndex, void> m_CleanWork;
  eng::threads::WorkSet<GlobalIndex, void> m_LightingWork;
  eng::threads::WorkSet<GlobalIndex, void> m_LazyMeshingWork;
  eng::threads::WorkSet<GlobalIndex, void> m_ForceMeshingWork;

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

  // Debug
  void loadChunk(const GlobalIndex& chunkIndex, block::ID blockType);

private:
  struct LightUniforms
  {
    const f32 maxSunlight = static_cast<f32>(block::Light::MaxValue());
    f32 sunIntensity = 1.0f;
  };

  void addToLightingUpdateQueue(const GlobalIndex& chunkIndex);
  void addToLazyMeshUpdateQueue(const GlobalIndex& chunkIndex);
  void addToForceMeshUpdateQueue(const GlobalIndex& chunkIndex);
  void addToMeshRemovalQueue(const GlobalIndex& chunkIndex);

  std::shared_ptr<Chunk> generateNewChunk(const GlobalIndex& chunkIndex);
  void eraseChunk(const GlobalIndex& chunkIndex);

  /*
    Queues chunk where the block update occured for updating. If specified block is on chunk border,
    will also update neighboring chunks. Chunk and its face neighbors are queue for an immediate update,
    while edge and corner neighbors are queued for later.
  */
  void sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  void uploadMeshes(eng::threads::UnorderedSet<ChunkDrawCommand>& commandQueue, std::unique_ptr<eng::MultiDrawIndexedArray<ChunkDrawCommand>>& multiDrawArray);

  /*
    Generates simplistic mesh in a compressed format based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.
    Uses AO algorithm outlined in https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
  */
  void meshChunk(const std::shared_ptr<Chunk>& chunk);

  void updateLighting(const std::shared_ptr<Chunk>& chunk);

  void lightingPacket(const GlobalIndex& chunkIndex);
  void lazyMeshingPacket(const GlobalIndex& chunkIndex);
  void forceMeshingPacket(const GlobalIndex& chunkIndex);
};