#pragma once
#include "VertexArray.h"
#include "MultiDrawArray.h"

namespace Engine
{
  namespace RenderCommand
  {
    void Initialize();
    void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    
    void Clear(const Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f });
    void WireFrameToggle(bool enableWireFrame);
    void FaceCullToggle(bool enableFaceCulling);
    
    void DrawVertices(const VertexArray* vertexArray, uint32_t vertexCount);
    void DrawIndexed(const VertexArray* vertexArray, uint32_t indexCount = 0);
    void DrawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount = 0);

    void MultiDrawImpl(const std::vector<DrawElementsIndirectCommand>& drawCommands);

    template<typename T>
    void MultiDrawIndexed(const MultiDrawArray<T>* multiDrawArray)
    {
      EN_CORE_ASSERT(multiDrawArray, "Vertex array has not been initialized!");
      multiDrawArray->bind();
      MultiDrawImpl(multiDrawArray->getQueuedDrawCommands());
    }

    void ClearDepthBuffer();
  };
}