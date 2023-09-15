#pragma once
#include "VertexArray.h"
#include "MultiDrawArray.h"

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
      OpenGL,
      OpenGL_Legacy
    };

    virtual ~RendererAPI() = default;

    virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

    virtual void clear(const Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) = 0;

    virtual void setBlendFunc() = 0;
    virtual void setBlending(bool enableBlending) = 0;
    virtual void setUseDepthOffset(bool enableDepthOffset) = 0;
    virtual void setDepthOffset(float factor, float units) = 0;
    virtual void setDepthTesting(bool enableDepthTesting) = 0;
    virtual void setDepthWriting(bool enableDepthWriting) = 0;
    virtual void setFaceCulling(bool enableFaceCulling) = 0;
    virtual void setWireFrame(bool enableWireFrame) = 0;

    virtual void drawVertices(const VertexArray* vertexArray, uint32_t vertexCount) = 0;
    virtual void drawIndexed(const VertexArray* vertexArray, uint32_t indexCount) = 0;
    virtual void drawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount) = 0;

    virtual void multiDrawVertices(const void* drawCommands, int drawCount, int stride) = 0;
    virtual void multiDrawIndexed(const void* drawCommands, int drawCount, int stride) = 0;

    virtual void clearDepthBuffer() = 0;

    static API GetAPI() { return s_API; }

  private:
    static inline API s_API = API::OpenGL_Legacy;
  };
}