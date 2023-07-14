#pragma once
#include "VertexArray.h"

namespace Engine
{
  namespace RenderCommand
  {
    void Initialize();
    void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    
    void Clear(const Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f });

    void SetBlending(bool enableBlending);
    void SetUseDepthOffset(bool enableDepthOffset);
    void SetDepthOffset(float factor, float units);
    void SetDepthTesting(bool enableDepthTesting);
    void SetDepthWriting(bool enableDepthWriting);
    void SetFaceCulling(bool enableFaceCulling);
    void SetWireFrame(bool enableWireFrame);
    
    void DrawVertices(const VertexArray* vertexArray, uint32_t vertexCount);
    void DrawIndexed(const VertexArray* vertexArray, uint32_t indexCount = 0);
    void DrawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount = 0);

    void MultiDrawVertices(const void* drawCommands, int drawCount, int stride);
    void MultiDrawIndexed(const void* drawCommands, int drawCount, int stride);

    void ClearDepthBuffer();
  };
}