#pragma once
#include "Engine/Memory/StorageBuffer.h"

namespace eng::mem
{
  class OpenGLStorageBuffer : public StorageBuffer
  {
    Type m_Type;
    uSize m_Size;
    u32 m_Binding;
    u32 m_BufferID;

  public:
    OpenGLStorageBuffer(Type type, u32 binding, uSize size);
    ~OpenGLStorageBuffer();

    Type type() const override;
    uSize size() const override;

    void write(const mem::RenderData& data) override;
  };
}