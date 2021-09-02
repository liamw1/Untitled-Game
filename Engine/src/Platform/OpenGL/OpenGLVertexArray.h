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

    void addVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) override;
    void setIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) override;

    const std::vector<std::shared_ptr<VertexBuffer>>& getBuffers() const override;
    const std::shared_ptr<IndexBuffer>& getIndexBuffer() const override;

  private:
    uint32_t m_RendererID;
    std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
  };
}