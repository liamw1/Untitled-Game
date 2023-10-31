#pragma once
#include "Engine/Renderer/StorageBuffer.h"

namespace eng
{
  class OpenGLStorageBuffer : public StorageBuffer
  {
  public:
    OpenGLStorageBuffer(Type type, std::optional<u32> binding);
    ~OpenGLStorageBuffer();

    void bind() const override;
    void unBind() const override;

    u32 size() const override;
    Type type() const override;

    void set(const void* data, u32 size) override;
    void update(const void* data, u32 offset, u32 size) override;
    void resize(u32 newSize) override;

  private:
    Type m_Type;
    u32 m_Size;
    std::optional<u32> m_Binding;
    u32 m_RendererID;
  };
}