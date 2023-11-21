#pragma once
#include "Engine/Memory/StorageBuffer.h"

namespace eng::mem
{
  class OpenGLStorageBuffer : public StorageBuffer
  {
    Type m_Type;
    u32 m_Size;
    std::optional<u32> m_Binding;
    u32 m_RendererID;

  public:
    OpenGLStorageBuffer(Type type, std::optional<u32> binding);
    ~OpenGLStorageBuffer();

    void bind() const override;
    void unBind() const override;

    u32 size() const override;
    Type type() const override;

    void set(const mem::Data& data) override;
    void modify(u32 offset, const mem::Data& data) override;
    void resize(u32 newSize) override;
  };
}