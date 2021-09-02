#pragma once
#include "Engine/Renderer/Buffer.h"

namespace Engine
{
  class OpenGLVertexBuffer : public VertexBuffer
  {
  public:
    OpenGLVertexBuffer(float* vertices, uint32_t size);
    ~OpenGLVertexBuffer();

    void bind() const override;
    void unBind() const override;

    inline const BufferLayout& getLayout() const override { return m_Layout; }
    inline void setLayout(const BufferLayout& layout) override { m_Layout = layout; }

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

    inline uint32_t getCount() const override { return m_Count; }

  private:
    uint32_t m_RendererID;
    uint32_t m_Count;
  };
}