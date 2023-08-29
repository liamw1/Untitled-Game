#pragma once
#include "Chunk.h"
#include "ChunkContainer.h"

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

  /*
    Unloads boundary chunks that are out of unload range.
  */
  void clean();

  void loadNewChunks();

  /*
    \returns The chunk along with a lock on its mutex. Chunk will be nullptr is no chunk is found.
  */
  [[nodiscard]] const ConstChunkWithLock acquireChunk(const LocalIndex& chunkIndex) const;

  void placeBlock(GlobalIndex chunkIndex, BlockIndex blockIndex, Direction face, Block::Type blockType);
  void removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  void setLoadModeTerrain();
  void setLoadModeVoid();

  // Debug
  void loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType);

private:
  enum class LoadMode
  {
    NotSet,
    Void,
    Terrain
  };

  // Rendering
  struct LightUniforms
  {
    const float maxSunlight = static_cast<float>(Block::Light::MaxValue());
    float sunIntensity = 1.0f;
  };

  static inline std::unique_ptr<Engine::Shader> s_Shader;
  static inline std::unique_ptr<Engine::Uniform> s_LightUniform;
  static inline std::unique_ptr<Engine::StorageBuffer> s_SSBO;
  static inline std::shared_ptr<Engine::TextureArray> s_TextureArray;
  static inline const Engine::BufferLayout s_VertexBufferLayout = { { ShaderDataType::Uint32, "a_VertexData" },
                                                                    { ShaderDataType::Uint32, "a_Lighting"   } };
  static constexpr int c_TextureSlot = 0;
  static constexpr int c_StorageBufferBinding = 0;
  static constexpr uint32_t c_StorageBufferSize = static_cast<uint32_t>(Engine::Pow2(20));

  Engine::Threads::UnorderedSet<Chunk::DrawCommand> m_OpaqueCommandQueue;
  Engine::Threads::UnorderedSet<Chunk::DrawCommand> m_TransparentCommandQueue;
  std::unique_ptr<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>> m_OpaqueMultiDrawArray;
  std::unique_ptr<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>> m_TransparentMultiDrawArray;

  // Multi-threading
  ChunkContainer m_ChunkContainer;
  std::shared_ptr<Engine::Threads::ThreadPool> m_ThreadPool;
  Engine::Threads::WorkSet<GlobalIndex, void> m_LoadWork;
  Engine::Threads::WorkSet<GlobalIndex, void> m_LightingWork;
  Engine::Threads::WorkSet<GlobalIndex, void> m_LazyMeshingWork;
  Engine::Threads::WorkSet<GlobalIndex, void> m_ForceMeshingWork;

  LoadMode m_LoadMode;
  GlobalIndex m_PrevPlayerOriginIndex;

  void addToLightingUpdateQueue(const GlobalIndex& chunkIndex);
  void addToLazyMeshUpdateQueue(const GlobalIndex& chunkIndex);
  void addToForceMeshUpdateQueue(const GlobalIndex& chunkIndex);

  void generateNewChunk(const GlobalIndex& chunkIndex);

  /*
    Queues chunk where the block update occured for updating. If specified block is on chunk border,
    will also update neighboring chunks. Chunk and its cardinal neighbors are queue for an immediate update,
    while edge and corner neighbors are queued for later.
  */
  void sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  // Meshing
private:
  struct BlockData
  {
    CubicArray<Block::Type, Chunk::Size() + 2, -1> composition;
    CubicArray<Block::Light, Chunk::Size() + 2, -1> lighting;

    BlockData();

    void fill(const BlockBox& fillSection, const Chunk* chunk, const BlockIndex& chunkBase, bool fillLight = true);

    static constexpr BlockBox Bounds() { return BlockBox(-1, Chunk::Size() + 1); }
  };

  BlockData& getBlockData(const GlobalIndex& chunkIndex, bool getInteriorLighting = true) const;

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
  void meshChunk(const GlobalIndex& chunkIndex);

  void updateLighting(const GlobalIndex& chunkIndex);

  void lightingPacket(const GlobalIndex& chunkIndex);
  void lazyMeshingPacket(const GlobalIndex& chunkIndex);
  void forceMeshingPacket(const GlobalIndex& chunkIndex);
};