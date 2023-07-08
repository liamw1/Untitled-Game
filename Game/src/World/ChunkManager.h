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

  /*
    \returns The chunk along with a lock on its mutex. Chunk will be nullptr is no chunk is found.
  */
  [[nodiscard]] std::pair<const Chunk*, std::unique_lock<std::mutex>> acquireChunk(const LocalIndex& chunkIndex) const;

  void placeBlock(GlobalIndex chunkIndex, BlockIndex blockIndex, Direction face, Block::Type blockType);
  void removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  void setLoadModeTerrain() { m_LoadMode = LoadMode::Terrain; }
  void setLoadModeVoid() { m_LoadMode = LoadMode::Void; }
  void launchLoadThread() { m_LoadThread = std::thread(&ChunkManager::loadWorker, this); }
  void launchUpdateThread() { m_UpdateThread = std::thread(&ChunkManager::updateWorker, this); }

  // Debug
  void loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType);

private:
  enum LoadMode
  {
    NotSet = 0,
    Void,
    Terrain
  };

  // Rendering
  struct UniformData
  {
    bool transparencyPass;
  };

  static inline std::unique_ptr<Engine::Shader> s_Shader;
  static inline std::unique_ptr<Engine::Uniform> s_Uniform;
  static inline std::unique_ptr<Engine::StorageBuffer> s_SSBO;
  static inline std::shared_ptr<Engine::TextureArray> s_TextureArray;
  static inline const Engine::BufferLayout s_VertexBufferLayout = { { ShaderDataType::Uint32, "a_VertexData" },
                                                                    { ShaderDataType::Uint32, "a_FaceData"   } };
  static constexpr int c_TextureSlot = 0;
  static constexpr int c_UniformBinding = 2;
  static constexpr int c_StorageBufferBinding = 0;
  static constexpr uint32_t c_StorageBufferSize = static_cast<uint32_t>(pow2(20));

  Threads::UnorderedSetQueue<Chunk::DrawCommand> m_OpaqueCommandQueue;
  std::unique_ptr<Engine::MultiDrawArray<Chunk::DrawCommand>> m_OpaqueMultiDrawArray;

  Threads::UnorderedSetQueue<Chunk::DrawCommand> m_TransparentCommandQueue;
  std::unique_ptr<Engine::MultiDrawArray<Chunk::DrawCommand>> m_TransparentMultiDrawArray;

  // Multi-threading
  std::atomic<bool> m_Running;
  std::thread m_LoadThread;
  std::thread m_UpdateThread;
  ChunkContainer m_ChunkContainer;

  LoadMode m_LoadMode;
  GlobalIndex m_PrevPlayerOriginIndex;

  void loadWorker();
  void updateWorker();

  void generateNewChunk(const GlobalIndex& chunkIndex);

  const Array3D<Block::Type, Chunk::Size() + 2>& getBlockData(const GlobalIndex& chunkIndex) const;

  /*
    Generates simplistic mesh in a compressed format based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.
    Compresed format is follows,
     bits 0-17:  Relative position of vertex within chunk (3-comps, 6 bits each)
     bits 18-19: Texture coordinate index (see Chunk.glsl for details)
     bits 20-21: Ambient Occlusion level (see link below)
     bits 22-31: Texure ID
     Uses AO algorithm outlined in https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
  */
  bool meshChunk(const GlobalIndex& chunkIndex);
};