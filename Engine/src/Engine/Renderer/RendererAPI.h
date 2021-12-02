#pragma once
#include <glm/glm.hpp>
#include "VertexArray.h"

/*
  API for basic renderer commands.
  Platform-specific implementation is determined by derived class.
*/
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
    virtual void wireFrameToggle(bool enableWireFrame) const = 0;
    virtual void faceCullToggle(bool enableFaceCulling) const = 0;
    
    virtual void drawIndexed(const Shared<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
    virtual void drawIndexedLines(const Shared<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

    static API GetAPI() { return s_API; }

  private:
    static API s_API;
  };
}