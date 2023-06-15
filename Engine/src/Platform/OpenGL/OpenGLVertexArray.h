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

    void clean() const override;

    void setLayout(const BufferLayout& layout) override;

    void setVertexBuffer(const void* data, uintptr_t size) const override;
    void setIndexBuffer(const std::shared_ptr<const IndexBuffer>& indexBuffer) override;

    const std::shared_ptr<const IndexBuffer>& getIndexBuffer() const override { return m_IndexBuffer; }

  private:
    uint32_t m_RendererID;
    uint32_t m_VertexBufferID;
    std::shared_ptr<const IndexBuffer> m_IndexBuffer;

    // Buffer for storing vertex buffer data set on a thread that is not the main thread
    mutable std::unique_ptr<char[]> m_VertexData = nullptr;
    mutable uintptr_t m_VertexDataSize = 0;

    void clearStoredVertexData() const;
  };
}