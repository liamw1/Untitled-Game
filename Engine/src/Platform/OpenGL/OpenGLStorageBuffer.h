#pragma once
#include "Engine/Renderer/StorageBuffer.h"

namespace Engine
{
  class OpenGLStorageBuffer : public StorageBuffer
  {
  public:
    OpenGLStorageBuffer(Type type, std::optional<uint32_t> binding);
    ~OpenGLStorageBuffer();

    void bind() const;
    void unBind() const;

    void set(const void* data, uint32_t size);
    void update(const void* data, uint32_t offset, uint32_t size);
    void resize(uint32_t newSize);

  private:
    Type m_Type;
    std::optional<uint32_t> m_Binding;
    uint32_t m_RendererID;
  };
}