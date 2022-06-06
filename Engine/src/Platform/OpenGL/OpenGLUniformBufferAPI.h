#pragma once
#include "Engine/Renderer/UniformBufferAPI.h"

namespace Engine
{
  class OpenGLUniformBufferAPI : public UniformBufferAPI
  {
  public:
    void allocate(uint32_t binding, uint32_t size);
    void deallocate(uint32_t binding);

    void bind(uint32_t binding) const;
    void unbind() const;

    uint32_t getSize(uint32_t binding) const;

    void setData(uint32_t binding, const void* data, uint32_t size, uint32_t offset);

  private:
    static constexpr int s_MaxUniformBindings = 32; // NOTE: Should query this from OpenGL
    std::array<uint32_t, s_MaxUniformBindings> m_RendererIDs;
    std::array<uint32_t, s_MaxUniformBindings> m_BufferSizes;
  };
}