#pragma once
#include "NewLOD.h"

namespace newLod
{
  class LODManager
  {
    // Rendering
    static constexpr i32 c_StorageBufferBinding = 1;
    static constexpr u32 c_StorageBufferSize = 0;
    static inline std::unique_ptr<eng::Shader> s_Shader;
    static inline std::unique_ptr<eng::mem::StorageBuffer> s_SSBO;
    static inline const eng::mem::BufferLayout s_VertexBufferLayout = { { eng::mem::ShaderDataType::Float3, "a_Position"        },
                                                                        { eng::mem::ShaderDataType::Float3, "a_IsoNormal"       },
                                                                        { eng::mem::ShaderDataType::Int2,   "a_TextureIndices"  },
                                                                        { eng::mem::ShaderDataType::Float2, "a_TextureWeighs"   },
                                                                        { eng::mem::ShaderDataType::Int,    "a_QuadIndex"       } };

    std::shared_ptr<eng::thread::AsyncMultiDrawArray<DrawCommand>> m_MultiDrawArray;

    // Multi-threading
    std::shared_ptr<eng::thread::ThreadPool> m_ThreadPool;
    eng::thread::WorkSet<GlobalIndex, void> m_MeshingWork;

    // LOD data
    Octree m_LODs;

  public:
    LODManager();

  private:
  };
}