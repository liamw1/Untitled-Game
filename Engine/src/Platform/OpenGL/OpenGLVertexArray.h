#pragma once
#include "Engine/Renderer/VertexArray.h"

namespace eng
{
  class OpenGLVertexArray : public VertexArray
  {
    u32 m_RendererID;
    BufferLayout m_VertexBufferLayout;
    std::optional<IndexBuffer> m_IndexBuffer;
    std::shared_ptr<StorageBuffer> m_VertexBuffer;

  public:
    OpenGLVertexArray();
    ~OpenGLVertexArray();

    void bind() const override;
    void unBind() const override;

    void setLayout(const BufferLayout& layout) override;

    void setVertexBuffer(const mem::Data& data) override;
    void setVertexBuffer(const std::shared_ptr<StorageBuffer>& vertexBuffer) override;
    void updateVertexBuffer(u32 offset, const mem::Data& data) const override;
    void resizeVertexBuffer(u32 newSize) override;

    void setIndexBuffer(const IndexBuffer& indexBuffer) override;
    void setIndexBuffer(const std::shared_ptr<StorageBuffer>& indexBufferStorage) override;

    const BufferLayout& getLayout() const override;
    const std::optional<IndexBuffer>& getIndexBuffer() const override;
  };
}