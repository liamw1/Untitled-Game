#pragma once
#include "Engine/Renderer/Buffer.h"

namespace Engine
{
  class OpenGLVertexBuffer : public VertexBuffer
  {
  public:
    OpenGLVertexBuffer(uint32_t size);
    OpenGLVertexBuffer(float* vertices, uint32_t size);
    ~OpenGLVertexBuffer();

    void bind() const override;
    void unBind() const override;

    const BufferLayout& getLayout() const override { return m_Layout; }
    void setLayout(const BufferLayout& layout) override { m_Layout = layout; }

    void setData(const void* data, uintptr_t size) override;

  private:
    uint32_t m_RendererID;
    BufferLayout m_Layout;
  };



  class OpenGLIndexBuffer : public IndexBuffer
  {
  public:
    OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
    ~OpenGLIndexBuffer();

    void bind() const override;
    void unBind() const override;

    uint32_t getCount() const override { return m_Count; }

  private:
    uint32_t m_RendererID;
    uint32_t m_Count;
  };
}