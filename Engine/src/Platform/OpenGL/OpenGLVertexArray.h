#pragma once
#include "Engine/Renderer/VertexArray.h"

namespace Engine
{
  class OpenGLIndexBuffer : public IndexBuffer
  {
  public:
    OpenGLIndexBuffer(const uint32_t* indices, uint32_t count);
    ~OpenGLIndexBuffer();

    void bind() const override;
    void unBind() const override;

    uint32_t getCount() const override { return m_Count; }

  private:
    uint32_t m_RendererID;
    uint32_t m_Count;
  };



  class OpenGLVertexArray : public VertexArray
  {
  public:
    OpenGLVertexArray();
    ~OpenGLVertexArray();

    void bind() const override;
    void unBind() const override;

    void setLayout(const BufferLayout& layout) override;

    void setVertexBuffer(const void* data, uintptr_t size) override;
    void setIndexBuffer(const Shared<const IndexBuffer>& indexBuffer) override;

    const Shared<const IndexBuffer>& getIndexBuffer() const override { return m_IndexBuffer; }

  private:
    uint32_t m_RendererID;
    uint32_t m_VertexBufferID;
    Shared<const IndexBuffer> m_IndexBuffer;
  };
}