#pragma once
#include "Engine/Memory/Data.h"
#include "Engine/Memory/StorageBuffer.h"

namespace eng
{
  class ShaderBufferStorage
  {
    static constexpr u32 c_MaxBindings = 36;
    static constexpr uSize c_MaxSize = std::numeric_limits<u32>::max();
    static inline std::array<std::unique_ptr<mem::StorageBuffer>, c_MaxBindings> s_SSBOs;

    u32 m_Binding;

  public:
    ShaderBufferStorage(u32 binding, uSize size);
    ~ShaderBufferStorage();

    void write(const mem::RenderData& data);
  };
}