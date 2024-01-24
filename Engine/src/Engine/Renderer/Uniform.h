#pragma once
#include "Engine/Memory/Data.h"
#include "Engine/Memory/StorageBuffer.h"

namespace eng
{
  class Uniform
  {
    static constexpr u32 c_MaxBindings = 36;
    static constexpr uSize c_MaxSize = 16384;
    static inline std::array<std::unique_ptr<mem::StorageBuffer>, c_MaxBindings> s_Uniforms;

    std::string m_Name;
    u32 m_Binding;

  public:
    Uniform(std::string_view name, u32 binding, uSize size);
    ~Uniform();

    std::string_view name() const;

    void write(const mem::UniformData& uniformData);
  };
}