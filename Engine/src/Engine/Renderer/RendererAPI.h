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

    virtual void initialize() = 0;
    virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

    virtual void clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) const = 0;
    
    virtual void drawIndexed(const Shared<VertexArray>& vertexArray) = 0;

    inline static API GetAPI() { return s_API; }

  private:
    static API s_API;
  };
}