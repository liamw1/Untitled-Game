#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace Engine
{
  class OpenGLRendererAPI : public RendererAPI
  {
  public:
    void init() override;
    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    void clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) const override;

    void drawIndexed(const Shared<VertexArray>& vertexArray) override;
  };
}