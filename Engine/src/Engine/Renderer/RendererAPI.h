#pragma once
#include "VertexArray.h"
#include "MultiDrawArray.h"

/*
  API for basic renderer commands.
  Platform-specific implementation is determined by derived class.
*/
namespace eng
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

    virtual void setViewport(u32 x, u32 y, u32 width, u32 height) = 0;

    virtual void clear(const math::Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) = 0;

    virtual void setBlendFunc() = 0;
    virtual void setBlending(bool enableBlending) = 0;
    virtual void setUseDepthOffset(bool enableDepthOffset) = 0;
    virtual void setDepthOffset(f32 factor, f32 units) = 0;
    virtual void setDepthTesting(bool enableDepthTesting) = 0;
    virtual void setDepthWriting(bool enableDepthWriting) = 0;
    virtual void setFaceCulling(bool enableFaceCulling) = 0;
    virtual void setWireFrame(bool enableWireFrame) = 0;

    virtual void drawVertices(const VertexArray* vertexArray, u32 vertexCount) = 0;
    virtual void drawIndexed(const VertexArray* vertexArray, u32 indexCount) = 0;
    virtual void drawIndexedLines(const VertexArray* vertexArray, u32 indexCount) = 0;

    virtual void multiDrawVertices(const void* drawCommands, i32 drawCount, i32 stride) = 0;
    virtual void multiDrawIndexed(const void* drawCommands, i32 drawCount, i32 stride) = 0;

    virtual void clearDepthBuffer() = 0;

    static API GetAPI() { return s_API; }

  private:
    static inline API s_API = API::OpenGL_Legacy;
  };
}