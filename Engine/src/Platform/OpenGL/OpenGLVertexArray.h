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

    void setVertexBuffer(const void* data, uint32_t size) override;

    void resizeVertexBuffer(uint32_t newSize) override;

    void modifyVertexBuffer(const void* data, uint32_t offset, uint32_t size) const override;

    void setIndexBuffer(const std::shared_ptr<const IndexBuffer>& indexBuffer) override;

    const std::shared_ptr<const IndexBuffer>& getIndexBuffer() const override { return m_IndexBuffer; }

  private:
    uint32_t m_RendererID;
    uint32_t m_VertexBufferID;
    BufferLayout m_VertexBufferLayout;
    std::shared_ptr<const IndexBuffer> m_IndexBuffer;
  };
}