#include "ENpch.h"
#include "OpenGLRendererAPI.h"
#include "Engine/Threads/Threads.h"
#include "Engine/Debug/Instrumentor.h"
#include <glad/glad.h>

namespace eng
{
  static void set(GLenum target, bool& currentValue, bool newValue)
  {
    if (newValue == currentValue)
      return;

    newValue ? glEnable(target) : glDisable(target);
    currentValue = newValue;
  }

  OpenGLRendererAPI::OpenGLRendererAPI()
    : m_BlendingEnabled(false),
      m_DepthOffsetEnabled(false),
      m_DepthTestingEnabled(false),
      m_DepthWritingEnabled(true),
      m_FaceCullingEnabled(false),
      m_WireFrameEnabled(false),
      m_DepthOffsetFactor(0.0f),
      m_DepthOffsetUnits(0.0f) {}

  void OpenGLRendererAPI::setViewport(u32 x, u32 y, u32 width, u32 height)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glViewport(x, y, width, height);
  }

  void OpenGLRendererAPI::clear(const math::Float4& color)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void OpenGLRendererAPI::clearDepthBuffer()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  void OpenGLRendererAPI::setBlendFunc()
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  void OpenGLRendererAPI::setBlending(bool enableBlending)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    set(GL_BLEND, m_BlendingEnabled, enableBlending);
  }

  void OpenGLRendererAPI::setUseDepthOffset(bool enableDepthOffset)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    set(GL_POLYGON_OFFSET_FILL, m_DepthOffsetEnabled, enableDepthOffset);
  }

  void OpenGLRendererAPI::setDepthOffset(f32 factor, f32 units)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    if (factor == m_DepthOffsetFactor && units == m_DepthOffsetUnits)
      return;

    glPolygonOffset(factor, units);
    m_DepthOffsetFactor = factor;
    m_DepthOffsetUnits = units;
  }

  void OpenGLRendererAPI::setDepthTesting(bool enableDepthTesting)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    set(GL_DEPTH_TEST, m_DepthTestingEnabled, enableDepthTesting);
  }

  void OpenGLRendererAPI::setDepthWriting(bool enableDepthWriting)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    if (enableDepthWriting == m_DepthWritingEnabled)
      return;

    enableDepthWriting ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
    m_DepthWritingEnabled = enableDepthWriting;
  }

  void OpenGLRendererAPI::setFaceCulling(bool enableFaceCulling)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    set(GL_CULL_FACE, m_FaceCullingEnabled, enableFaceCulling);
  }

  void OpenGLRendererAPI::setWireFrame(bool enableWireFrame)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    if (enableWireFrame == m_WireFrameEnabled)
      return;

    enableWireFrame ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_WireFrameEnabled = enableWireFrame;
  }

  void OpenGLRendererAPI::drawVertices(const VertexArray* vertexArray, u32 vertexCount)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(vertexArray, "Vertex array has not been initialized!");

    vertexArray->bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
  }

  void OpenGLRendererAPI::drawIndexed(const VertexArray* vertexArray, u32 indexCount)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(vertexArray, "Vertex array has not been initialized!");

    vertexArray->bind();
    u32 count = indexCount == 0 ? vertexArray->getIndexBuffer()->count() : indexCount;
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
  }

  void OpenGLRendererAPI::drawIndexedLines(const VertexArray* vertexArray, u32 indexCount)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(vertexArray, "Vertex array has not been initialized!");

    vertexArray->bind();
    u32 count = indexCount == 0 ? vertexArray->getIndexBuffer()->count() : indexCount;
    glDrawElements(GL_LINES, count, GL_UNSIGNED_INT, nullptr);
  }

  void OpenGLRendererAPI::multiDrawVertices(const void* drawCommands, i32 drawCount, i32 stride)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glMultiDrawArraysIndirect(GL_TRIANGLES, drawCommands, static_cast<GLsizei>(drawCount), stride);
  }

  void OpenGLRendererAPI::multiDrawIndexed(const void* drawCommands, i32 drawCount, i32 stride)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, drawCommands, static_cast<GLsizei>(drawCount), stride);
  }
}