#pragma once
#include "Engine/Memory/Data.h"
#include "Engine/Memory/Buffer.h"

namespace eng
{
  class Uniform
  {
    static constexpr u32 c_MaxBindings = 36;
    static constexpr uSize c_MaxSize = 16384;
    static inline std::array<std::unique_ptr<mem::StorageBuffer>, c_MaxBindings> s_Uniforms;

    u32 m_Binding;

  public:
    Uniform(u32 binding, uSize size);
    ~Uniform();

    void set(const mem::UniformData& uniformData);
  };
}