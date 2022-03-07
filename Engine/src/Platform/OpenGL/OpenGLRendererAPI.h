#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace Engine
{
  class OpenGLRendererAPI : public RendererAPI
  {
  public:
    void initialize() override;
    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    void clear(const Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) override;
    void wireFrameToggle(bool enableWireFrame) override;
    void faceCullToggle(bool enableFaceCulling) override;

    void drawVertices(const Shared<VertexArray>& vertexArray, uint32_t vertexCount) override;
    void drawIndexed(const Shared<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
    void drawIndexedLines(const Shared<VertexArray>& vertexArray, uint32_t indexCount = 0) override;

    void clearDepthBuffer() override;
  };
}