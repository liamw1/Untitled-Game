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
      default:                              throw  std::invalid_argument("Unknown RendererAPI!");
    }
  }
  static std::unique_ptr<RendererAPI> s_RendererAPI = createAPI();

  void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { s_RendererAPI->setViewport(x, y, width, height); }

  void clear(const math::Float4& color) { s_RendererAPI->clear(color); }
  void clearDepthBuffer() { s_RendererAPI->clearDepthBuffer(); }

  void setBlendFunc() { s_RendererAPI->setBlendFunc(); }
  void setBlending(bool enableBlending) { s_RendererAPI->setBlending(enableBlending); }
  void setUseDepthOffset(bool enableDepthOffset) { s_RendererAPI->setUseDepthOffset(enableDepthOffset); }
  void setDepthOffset(float factor, float units) { s_RendererAPI->setDepthOffset(factor, units); }
  void setDepthTesting(bool enableDepthTesting) { s_RendererAPI->setDepthTesting(enableDepthTesting); }
  void setDepthWriting(bool enableDepthWriting) { s_RendererAPI->setDepthWriting(enableDepthWriting); }
  void setFaceCulling(bool enableFaceCulling) { s_RendererAPI->setFaceCulling(enableFaceCulling); }
  void setWireFrame(bool enableWireFrame) { s_RendererAPI->setWireFrame(enableWireFrame); }

  void drawVertices(const VertexArray* vertexArray, uint32_t vertexCount) { s_RendererAPI->drawVertices(vertexArray, vertexCount); }
  void drawIndexed(const VertexArray* vertexArray, uint32_t indexCount) { s_RendererAPI->drawIndexed(vertexArray, indexCount); }
  void drawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount) { s_RendererAPI->drawIndexedLines(vertexArray, indexCount); }

  void multiDrawVertices(const void* drawCommands, int drawCount, int stride) { s_RendererAPI->multiDrawVertices(drawCommands, drawCount, stride); }
  void multiDrawIndexed(const void* drawCommands, int drawCount, int stride) { s_RendererAPI->multiDrawIndexed(drawCommands, drawCount, stride); }
}