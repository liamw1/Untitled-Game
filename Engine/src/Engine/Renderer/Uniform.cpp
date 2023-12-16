#include "ENpch.h"
#include "Uniform.h"
#include "RendererAPI.h"

namespace eng
{
  Uniform::Uniform(u32 binding, uSize size)
    : m_Binding(binding)
  {
    ENG_CORE_ASSERT(m_Binding < c_MaxUniformBindings, "Binding exceeds maximum allowed uniform bindings!");

    if (size > c_MaxSize)
    {
      ENG_CORE_ERROR("Requested uniform size is larger than the maximum allowable uniform block size!");
      return;
    }
    if (s_Uniforms[m_Binding])
      ENG_CORE_WARN("Uniform buffer has already been allocated at binding {0}!", m_Binding);

    s_Uniforms[m_Binding] = mem::StorageBuffer::Create(mem::StorageBuffer::Type::Uniform, m_Binding, size);
  }

  Uniform::~Uniform()
  {
    s_Uniforms[m_Binding].reset();
  }

  void Uniform::set(const mem::UniformData& uniformData)
  {
    s_Uniforms[m_Binding]->set(static_cast<mem::RenderData>(uniformData));
  }
}