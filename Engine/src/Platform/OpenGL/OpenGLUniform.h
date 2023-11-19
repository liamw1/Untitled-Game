#pragma once
#include "Engine/Renderer/Uniform.h"

namespace eng
{
  class OpenGLUniform : public Uniform
  {
    static constexpr u32 c_MaxUniformBindings = 36;
    static constexpr u32 c_MaxUniformBlockSize = 16384;

    static inline std::array<u32, c_MaxUniformBindings> s_BufferSizes{};
    static inline std::array<u32, c_MaxUniformBindings> s_RendererIDs{};

    u32 m_Binding;

  public:
    OpenGLUniform(u32 binding, u32 size);
    ~OpenGLUniform();

    void bind() const override;
    void unBind() const override;

    void set(const mem::UniformData& uniformData) override;
  };
}