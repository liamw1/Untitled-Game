#pragma once
#include "VertexArray.h"

namespace eng::command
{
  void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
  
  void clear(const math::Float4& color = { 0.0f, 0.0f, 0.0f, 1.0f });
  void clearDepthBuffer();

  void setBlendFunc();
  void setBlending(bool enableBlending);
  void setUseDepthOffset(bool enableDepthOffset);
  void setDepthOffset(float factor, float units);
  void setDepthTesting(bool enableDepthTesting);
  void setDepthWriting(bool enableDepthWriting);
  void setFaceCulling(bool enableFaceCulling);
  void setWireFrame(bool enableWireFrame);
  
  void drawVertices(const VertexArray* vertexArray, uint32_t vertexCount);
  void drawIndexed(const VertexArray* vertexArray, uint32_t indexCount = 0);
  void drawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount = 0);

  void multiDrawVertices(const void* drawCommands, int drawCount, int stride);
  void multiDrawIndexed(const void* drawCommands, int drawCount, int stride);
}