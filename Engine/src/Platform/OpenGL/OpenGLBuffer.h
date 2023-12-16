#pragma once
#include "Engine/Memory/Buffer.h"

namespace eng::mem
{
  class OpenGLDynamicBuffer : public DynamicBuffer
  {
    Type m_Type;
    uSize m_Size;
    u32 m_RendererID;

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

  class OpenGLStorageBuffer : public StorageBuffer
  {
    Type m_Type;
    uSize m_Size;
    uSize m_Capacity;
    u32 m_Binding;
    u32 m_RendererID;

  public:
    OpenGLStorageBuffer(Type type, u32 binding, uSize capacity);
    ~OpenGLStorageBuffer();

    Type type() const override;
    uSize size() const override;
    uSize capacity() const override;

    void set(const mem::RenderData& data) override;
  };
}