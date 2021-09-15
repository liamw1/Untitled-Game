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

    void addVertexBuffer(const Shared<VertexBuffer>& vertexBuffer) override;
    void setIndexBuffer(const Shared<IndexBuffer>& indexBuffer) override;

    const std::vector<Shared<VertexBuffer>>& getBuffers() const override;
    const Shared<IndexBuffer>& getIndexBuffer() const override;

  private:
    uint32_t m_RendererID;
    std::vector<Shared<VertexBuffer>> m_VertexBuffers;
    Shared<IndexBuffer> m_IndexBuffer;
  };
}