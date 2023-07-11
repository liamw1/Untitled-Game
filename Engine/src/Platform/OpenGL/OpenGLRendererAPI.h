#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace Engine
{
  class OpenGLRendererAPI : public RendererAPI
  {
  public:
    OpenGLRendererAPI();

    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    void clear(const Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) override;

    void setBlending(bool enableBlending) override;
    void setUseDepthOffset(bool enableDepthOffset) override;
    void setDepthOffset(float factor, float units) override;
    void setDepthTesting(bool enableDepthTesting) override;
    void setDepthWriting(bool enableDepthWriting) override;
    void setFaceCulling(bool enableFaceCulling) override;
    void setWireFrame(bool enableWireFrame) override;

    void drawVertices(const VertexArray* vertexArray, uint32_t vertexCount) override;
    void drawIndexed(const VertexArray* vertexArray, uint32_t indexCount) override;
    void drawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount) override;

    void multiDrawVertices(const void* drawCommands, int drawCount, int stride) override;

    void clearDepthBuffer() override;

  private:
    bool m_BlendingEnabled;
    bool m_DepthOffsetEnabled;
    bool m_DepthTestingEnabled;
    bool m_DepthWritingEnabled;
    bool m_FaceCullingEnabled;
    bool m_WireFrameEnabled;

    float m_DepthOffsetFactor;
    float m_DepthOffsetUnits;
  };
}