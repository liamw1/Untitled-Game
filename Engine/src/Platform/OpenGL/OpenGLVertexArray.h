#pragma once
#include "Engine/Renderer/VertexArray.h"

namespace eng
{
  class OpenGLVertexArray : public VertexArray
  {
    u32 m_RendererID;
    mem::BufferLayout m_VertexBufferLayout;
    std::optional<IndexBuffer> m_IndexBuffer;
    std::shared_ptr<mem::StorageBuffer> m_VertexBuffer;

  public:
    OpenGLVertexArray();
    ~OpenGLVertexArray();

    void bind() const override;
    void unBind() const override;

    void setLayout(const mem::BufferLayout& layout) override;

    void setVertexBuffer(const mem::RenderData& data) override;
    void setVertexBuffer(const std::shared_ptr<mem::StorageBuffer>& vertexBuffer) override;
    void modifyVertexBuffer(u32 offset, const mem::RenderData& data) const override;
    void resizeVertexBuffer(u32 newSize) override;

    void setIndexBuffer(const IndexBuffer& indexBuffer) override;
    void setIndexBuffer(const std::shared_ptr<mem::StorageBuffer>& indexBufferStorage) override;

    const mem::BufferLayout& getLayout() const override;
    const std::optional<IndexBuffer>& getIndexBuffer() const override;
  };
}