#pragma once
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/MultiDrawArray.h"

namespace Engine
{
  class OpenGLRendererAPI : public RendererAPI
  {
  public:
    OpenGLRendererAPI();

    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    void clear(const Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) override;
    void wireFrameToggle(bool enableWireFrame) override;
    void faceCullToggle(bool enableFaceCulling) override;

    void drawVertices(const VertexArray* vertexArray, uint32_t vertexCount) override;
    void drawIndexed(const VertexArray* vertexArray, uint32_t indexCount) override;
    void drawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount) override;
    void multiDrawIndexed(const MultiDrawArray* multiDrawArray) override;

    void clearDepthBuffer() override;
  };
}