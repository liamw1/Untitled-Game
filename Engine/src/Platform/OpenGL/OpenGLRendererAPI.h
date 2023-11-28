#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace eng
{
  class OpenGLRendererAPI : public RendererAPI
  {
    bool m_BlendingEnabled;
    bool m_DepthOffsetEnabled;
    bool m_DepthTestingEnabled;
    bool m_DepthWritingEnabled;
    bool m_FaceCullingEnabled;
    bool m_WireFrameEnabled;

    f32 m_DepthOffsetFactor;
    f32 m_DepthOffsetUnits;

  public:
    OpenGLRendererAPI();

    void setViewport(u32 x, u32 y, u32 width, u32 height) override;

    void clear(const math::Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) override;
    void clearDepthBuffer() override;

    void setBlending(bool enableBlending) override;
    void setUseDepthOffset(bool enableDepthOffset) override;
    void setDepthOffset(f32 factor, f32 units) override;
    void setDepthTesting(bool enableDepthTesting) override;
    void setDepthWriting(bool enableDepthWriting) override;
    void setFaceCulling(bool enableFaceCulling) override;
    void setWireFrame(bool enableWireFrame) override;

    void drawVertices(const VertexArray* vertexArray, u32 vertexCount) override;
    void drawIndexed(const VertexArray* vertexArray, u32 indexCount) override;
    void drawIndexedLines(const VertexArray* vertexArray, u32 indexCount) override;

    void multiDrawVertices(const mem::Data& drawCommandData, i32 commandCount) override;
    void multiDrawIndexed(const mem::Data& drawCommandData, i32 commandCount) override;
  };
}