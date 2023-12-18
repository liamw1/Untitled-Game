#pragma once
#include "Engine/Renderer/VertexArray.h"

namespace eng
{
  class OpenGLVertexArray : public VertexArray
  {
    u32 m_VertexArrayID;
    mem::BufferLayout m_VertexBufferLayout;
    std::optional<IndexBuffer> m_IndexBuffer;
    std::shared_ptr<mem::DynamicBuffer> m_VertexBuffer;

  public:
    OpenGLVertexArray();
    ~OpenGLVertexArray();

    void bind() const override;
    void unbind() const override;

    void setLayout(const mem::BufferLayout& layout) override;

    void setVertexBuffer(const mem::RenderData& data) override;
    void setVertexBuffer(const std::shared_ptr<mem::DynamicBuffer>& vertexBuffer) override;
    void modifyVertexBuffer(u32 offset, const mem::RenderData& data) override;
    void resizeVertexBuffer(uSize newSize) override;

    void setIndexBuffer(const IndexBuffer& indexBuffer) override;
    void setIndexBuffer(const std::shared_ptr<mem::DynamicBuffer>& indexBuffer) override;

    const mem::BufferLayout& layout() const override;
    const std::optional<IndexBuffer>& indexBuffer() const override;
  };
}