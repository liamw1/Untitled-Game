#pragma once
#include "Engine/Renderer/UniformBufferAPI.h"

namespace Engine
{
  class OpenGLUniformBufferAPI : public UniformBufferAPI
  {
  public:
    OpenGLUniformBufferAPI();

    void allocate(uint32_t binding, uint32_t size) override;
    void deallocate(uint32_t binding) override;

    void bind(uint32_t binding) const override;
    void unbind() const override;

    int maxSize() const override;
    int getSize(uint32_t binding) const override;

    void setData(uint32_t binding, const void* data, uint32_t size, uint32_t offset) override;

  private:
    static constexpr int c_MaxUniformBindings = 36;
    std::array<uint32_t, c_MaxUniformBindings> m_BufferSizes{};
    std::array<uint32_t, c_MaxUniformBindings> m_RendererIDs{};
  };
}