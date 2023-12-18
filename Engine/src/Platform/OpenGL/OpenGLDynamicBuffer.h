#pragma once
#include "Engine/Memory/DynamicBuffer.h"

namespace eng::mem
{
  class OpenGLDynamicBuffer : public DynamicBuffer
  {
    Type m_Type;
    uSize m_Size;
    u32 m_BufferID;

  public:
    OpenGLDynamicBuffer(Type type);
    ~OpenGLDynamicBuffer();

    void bind() const override;
    void unbind() const override;

    Type type() const override;
    uSize size() const override;

    void set(const mem::RenderData& data) override;
    void modify(u32 offset, const mem::RenderData& data) override;
    void resize(uSize newSize) override;
  };
}