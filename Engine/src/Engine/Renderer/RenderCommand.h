#pragma once
#include "VertexArray.h"
#include "Engine/Math/Vec.h"

namespace eng::render::command
{
  void setViewport(u32 x, u32 y, u32 width, u32 height);
  
  void clear(const math::Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f });
  void clearDepthBuffer();

  void setBlending(bool enableBlending);
  void setUseDepthOffset(bool enableDepthOffset);
  void setDepthOffset(f32 factor, f32 units);
  void setDepthTesting(bool enableDepthTesting);
  void setDepthWriting(bool enableDepthWriting);
  void setFaceCulling(bool enableFaceCulling);
  void setWireFrame(bool enableWireFrame);
  
  void drawVertices(const VertexArray& vertexArray, uSize vertexCount);
  void drawIndexed(const VertexArray& vertexArray, uSize indexCount = 0);
  void drawIndexedLines(const VertexArray& vertexArray, uSize indexCount = 0);

  void multiDrawVertices(const mem::RenderData& drawCommandData, uSize commandCount);
  void multiDrawIndexed(const mem::RenderData& drawCommandData, uSize commandCount);
}