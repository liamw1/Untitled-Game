#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace Engine
{
  class OpenGLRendererAPI : public RendererAPI
  {
  public:
    void Init() override;

    void Clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) const override;

    void DrawIndexed(const Shared<VertexArray>& vertexArray) override;
  };
}