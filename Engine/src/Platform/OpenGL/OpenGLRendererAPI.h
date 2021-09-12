#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace Engine
{
  class OpenGLRendererAPI : public RendererAPI
  {
  public:
    void Clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) const override;

    void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) override;
  };
}