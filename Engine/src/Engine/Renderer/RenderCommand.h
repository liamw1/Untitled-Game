#pragma once
#include "RendererAPI.h"

namespace Engine
{
  class RenderCommand
  {
  public:
    inline static void Clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) { s_RendererAPI->Clear(color); }

    inline static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) { return s_RendererAPI->DrawIndexed(vertexArray); }

  private:
    static RendererAPI* s_RendererAPI;
  };
}