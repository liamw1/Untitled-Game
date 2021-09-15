#pragma once
#include <glm/glm.hpp>
#include "VertexArray.h"

namespace Engine
{
  class RendererAPI
  {
  public:
    enum class API
    {
      None,
      OpenGL
    };

    virtual void Clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) const = 0;

    virtual void DrawIndexed(const Shared<VertexArray>& vertexArray) = 0;

    inline static API GetAPI() { return s_API; }

  private:
    static API s_API;
  };
}