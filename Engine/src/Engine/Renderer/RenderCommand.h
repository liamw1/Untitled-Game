#pragma once
#include "RendererAPI.h"

namespace Engine
{
  class RenderCommand
  {
  public:
    static void Initialize() { s_RendererAPI->initialize(); }
    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { s_RendererAPI->setViewport(x, y, width, height); }
    
    static void Clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) { s_RendererAPI->clear(color); }
    static void WireFrameToggle(bool enableWireFrame) { s_RendererAPI->wireFrameToggle(enableWireFrame); }
    static void FaceCullToggle(bool enableFaceCulling) { s_RendererAPI->faceCullToggle(enableFaceCulling); }
    
    static void DrawIndexed(const Shared<VertexArray>& vertexArray, uint32_t indexCount = 0) { return s_RendererAPI->drawIndexed(vertexArray, indexCount); }

  private:
    static Unique<RendererAPI> s_RendererAPI;
  };
}