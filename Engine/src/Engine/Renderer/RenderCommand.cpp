#include "ENpch.h"
#include "RenderCommand.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Engine
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

  void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
  {
    s_RendererAPI->setViewport(x, y, width, height);
  }

  void RenderCommand::Clear(const Float4& color)
  {
    s_RendererAPI->clear(color);
  }

  void RenderCommand::SetBlendFunc()
  {
    s_RendererAPI->setBlendFunc();
  }

  void RenderCommand::SetBlending(bool enableBlending)
  {
    s_RendererAPI->setBlending(enableBlending);
  }

  void RenderCommand::SetUseDepthOffset(bool enableDepthOffset)
  {
    s_RendererAPI->setUseDepthOffset(enableDepthOffset);
  }

  void RenderCommand::SetDepthOffset(float factor, float units)
  {
    s_RendererAPI->setDepthOffset(factor, units);
  }

  void RenderCommand::SetDepthTesting(bool enableDepthTesting)
  {
    s_RendererAPI->setDepthTesting(enableDepthTesting);
  }

  void RenderCommand::SetDepthWriting(bool enableDepthWriting)
  {
    s_RendererAPI->setDepthWriting(enableDepthWriting);
  }

  void RenderCommand::SetFaceCulling(bool enableFaceCulling)
  {
    s_RendererAPI->setFaceCulling(enableFaceCulling);
  }

  void RenderCommand::SetWireFrame(bool enableWireFrame)
  {
    s_RendererAPI->setWireFrame(enableWireFrame);
  }

  void RenderCommand::DrawVertices(const VertexArray* vertexArray, uint32_t vertexCount)
  {
    s_RendererAPI->drawVertices(vertexArray, vertexCount);
  }

  void RenderCommand::DrawIndexed(const VertexArray* vertexArray, uint32_t indexCount)
  {
    s_RendererAPI->drawIndexed(vertexArray, indexCount);
  }

  void RenderCommand::DrawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount)
  {
    s_RendererAPI->drawIndexedLines(vertexArray, indexCount);
  }

  void RenderCommand::MultiDrawVertices(const void* drawCommands, int drawCount, int stride)
  {
    s_RendererAPI->multiDrawVertices(drawCommands, drawCount, stride);
  }

  void RenderCommand::MultiDrawIndexed(const void* drawCommands, int drawCount, int stride)
  {
    s_RendererAPI->multiDrawIndexed(drawCommands, drawCount, stride);
  }

  void RenderCommand::ClearDepthBuffer()
  {
    s_RendererAPI->clearDepthBuffer();
  }
}