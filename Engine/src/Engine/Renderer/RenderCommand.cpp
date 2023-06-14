#include "ENpch.h"
#include "RenderCommand.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Engine
{
  static std::unique_ptr<RendererAPI> s_RendererAPI = nullptr;

  void RenderCommand::Initialize()
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return;
      case RendererAPI::API::OpenGL:        s_RendererAPI = std::make_unique<OpenGLRendererAPI>();  break;
      case RendererAPI::API::OpenGL_Legacy: s_RendererAPI = std::make_unique<OpenGLRendererAPI>();  break;
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!");      return;
    }
  }

  void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
  {
    s_RendererAPI->setViewport(x, y, width, height);
  }

  void RenderCommand::Clear(const Float4& color)
  {
    s_RendererAPI->clear(color);
  }

  void RenderCommand::WireFrameToggle(bool enableWireFrame)
  {
    s_RendererAPI->wireFrameToggle(enableWireFrame);
  }

  void RenderCommand::FaceCullToggle(bool enableFaceCulling)
  {
    s_RendererAPI->faceCullToggle(enableFaceCulling);
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

  void RenderCommand::ClearDepthBuffer()
  {
    s_RendererAPI->clearDepthBuffer();
  }
}