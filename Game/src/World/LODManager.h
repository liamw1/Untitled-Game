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
    static inline std::unique_ptr<eng::StorageBuffer> s_SSBO;
    static inline const eng::BufferLayout s_VertexBufferLayout = { { eng::ShaderDataType::Float3, "a_Position"        },
                                                                   { eng::ShaderDataType::Float3, "a_IsoNormal"       },
                                                                   { eng::ShaderDataType::Int2,   "a_TextureIndices"  },
                                                                   { eng::ShaderDataType::Float2, "a_TextureWeighs"   },
                                                                   { eng::ShaderDataType::Int,    "a_QuadIndex"       } };

    eng::thread::UnorderedSet<DrawCommand> m_CommandQueue;
    eng::MultiDrawIndexedArray<DrawCommand> m_MultiDrawArray;

    // LOD data
    Octree m_LODs;

    // Multi-threading

  public:
    LODManager();

  private:
  };
}