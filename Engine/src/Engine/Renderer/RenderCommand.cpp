#include "ENpch.h"
#include "RenderCommand.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace eng::render::command
{
  static std::unique_ptr<RendererAPI> createAPI()
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLRendererAPI>();
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLRendererAPI>();
    }
    throw std::invalid_argument("Invalid RendererAPI!");
  }
  static std::unique_ptr<RendererAPI> s_RendererAPI = createAPI();

  void setViewport(u32 x, u32 y, u32 width, u32 height) { s_RendererAPI->setViewport(x, y, width, height); }

  void clear(const math::Float4& color) { s_RendererAPI->clear(color); }
  void clearDepthBuffer() { s_RendererAPI->clearDepthBuffer(); }

  void setBlendFunc() { s_RendererAPI->setBlendFunc(); }
  void setBlending(bool enableBlending) { s_RendererAPI->setBlending(enableBlending); }
  void setUseDepthOffset(bool enableDepthOffset) { s_RendererAPI->setUseDepthOffset(enableDepthOffset); }
  void setDepthOffset(f32 factor, f32 units) { s_RendererAPI->setDepthOffset(factor, units); }
  void setDepthTesting(bool enableDepthTesting) { s_RendererAPI->setDepthTesting(enableDepthTesting); }
  void setDepthWriting(bool enableDepthWriting) { s_RendererAPI->setDepthWriting(enableDepthWriting); }
  void setFaceCulling(bool enableFaceCulling) { s_RendererAPI->setFaceCulling(enableFaceCulling); }
  void setWireFrame(bool enableWireFrame) { s_RendererAPI->setWireFrame(enableWireFrame); }

  void drawVertices(const VertexArray* vertexArray, u32 vertexCount) { s_RendererAPI->drawVertices(vertexArray, vertexCount); }
  void drawIndexed(const VertexArray* vertexArray, u32 indexCount) { s_RendererAPI->drawIndexed(vertexArray, indexCount); }
  void drawIndexedLines(const VertexArray* vertexArray, u32 indexCount) { s_RendererAPI->drawIndexedLines(vertexArray, indexCount); }

  void multiDrawVertices(const void* drawCommands, i32 drawCount, i32 stride) { s_RendererAPI->multiDrawVertices(drawCommands, drawCount, stride); }
  void multiDrawIndexed(const void* drawCommands, i32 drawCount, i32 stride) { s_RendererAPI->multiDrawIndexed(drawCommands, drawCount, stride); }
}