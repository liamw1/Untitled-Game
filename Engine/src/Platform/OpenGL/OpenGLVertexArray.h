#pragma once
#include "Engine/Renderer/VertexArray.h"

namespace Engine
{
  class OpenGLVertexArray : public VertexArray
  {
  public:
    OpenGLVertexArray();
    ~OpenGLVertexArray();

    void bind() const override;
    void unBind() const override;

    void setLayout(const BufferLayout& layout) override;

    void setVertexData(const void* data, uintptr_t size) override;
    void setIndexBuffer(const uint32_t* indices, uint32_t indexCount) override;

    uint32_t getIndexCount() const override { return m_IndexCount; }

  private:
    uint32_t m_RendererID;
    uint32_t m_VertexBufferID;
    uint32_t m_IndexBufferID = 0;
    uint32_t m_IndexCount = 0;
  };
}