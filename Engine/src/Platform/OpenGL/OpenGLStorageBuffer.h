#pragma once
#include "Engine/Renderer/StorageBuffer.h"

namespace eng
{
  class OpenGLStorageBuffer : public StorageBuffer
  {
  public:
    OpenGLStorageBuffer(Type type, std::optional<uint32_t> binding);
    ~OpenGLStorageBuffer();

    void bind() const override;
    void unBind() const override;

    uint32_t size() const override;
    Type type() const override;

    void set(const void* data, uint32_t size) override;
    void update(const void* data, uint32_t offset, uint32_t size) override;
    void resize(uint32_t newSize) override;

  private:
    Type m_Type;
    uint32_t m_Size;
    std::optional<uint32_t> m_Binding;
    uint32_t m_RendererID;
  };
}