#pragma once
#include "Engine/Renderer/Uniform.h"

namespace eng
{
  class OpenGLUniform : public Uniform
  {
  public:
    OpenGLUniform(uint32_t binding, uint32_t size);
    ~OpenGLUniform();

    void bind() const override;
    void unBind() const override;

    void set(const void* data, uint32_t size) override;

  private:
    static constexpr uint32_t c_MaxUniformBindings = 36;
    static constexpr uint32_t c_MaxUniformBlockSize = 16384;

    static inline std::array<uint32_t, c_MaxUniformBindings> s_BufferSizes{};
    static inline std::array<uint32_t, c_MaxUniformBindings> s_RendererIDs{};

    uint32_t m_Binding;
  };
}