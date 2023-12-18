#include "ENpch.h"
#include "SSBO.h"
#include "RendererAPI.h"

namespace eng
{
  ShaderBufferStorage::ShaderBufferStorage(u32 binding, uSize size)
    : m_Binding(binding)
  {
    ENG_CORE_ASSERT(m_Binding < c_MaxBindings, "Binding exceeds maximum allowed SSBO bindings!");

    if (size > c_MaxSize)
    {
      ENG_CORE_ERROR("Requested SSBO size is larger than the maximum allowable SSBO size!");
      return;
    }
    if (s_SSBOs[m_Binding])
      ENG_CORE_WARN("SSBO has already been allocated at binding {0}!", m_Binding);

    s_SSBOs[m_Binding] = mem::StorageBuffer::Create(mem::StorageBuffer::Type::SSBO, m_Binding, size);
  }

  ShaderBufferStorage::~ShaderBufferStorage()
  {
    s_SSBOs[m_Binding].reset();
  }

  void ShaderBufferStorage::write(const mem::RenderData& data)
  {
    s_SSBOs[m_Binding]->write(data);
  }
}
