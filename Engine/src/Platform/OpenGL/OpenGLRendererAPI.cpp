#include "ENpch.h"
#include "OpenGLRendererAPI.h"
#include "Engine/Threading/Threads.h"
#include "Engine/Debug/Instrumentor.h"
#include <glad/glad.h>

namespace Engine
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
      m_DepthOffsetUnits(0.0f)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setBlending(true);
    setDepthTesting(true);

    // glEnable(GL_MULTISAMPLE);
  }

  void OpenGLRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glViewport(x, y, width, height);
  }

  void OpenGLRendererAPI::clear(const Float4& color)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void OpenGLRendererAPI::setBlending(bool enableBlending)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    set(GL_BLEND, m_BlendingEnabled, enableBlending);
  }

  void OpenGLRendererAPI::setUseDepthOffset(bool enableDepthOffset)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    set(GL_POLYGON_OFFSET_FILL, m_DepthOffsetEnabled, enableDepthOffset);
  }

  void OpenGLRendererAPI::setDepthOffset(float factor, float units)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    if (factor == m_DepthOffsetFactor && units == m_DepthOffsetUnits)
      return;

    glPolygonOffset(factor, units);
    m_DepthOffsetFactor = factor;
    m_DepthOffsetUnits = units;
  }

  void OpenGLRendererAPI::setDepthTesting(bool enableDepthTesting)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    set(GL_DEPTH_TEST, m_DepthTestingEnabled, enableDepthTesting);
  }

  void OpenGLRendererAPI::setDepthWriting(bool enableDepthWriting)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    if (enableDepthWriting == m_DepthWritingEnabled)
      return;

    enableDepthWriting ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
    m_DepthWritingEnabled = enableDepthWriting;
  }

  void OpenGLRendererAPI::setFaceCulling(bool enableFaceCulling)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    set(GL_CULL_FACE, m_FaceCullingEnabled, enableFaceCulling);
  }

  void OpenGLRendererAPI::setWireFrame(bool enableWireFrame)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    if (enableWireFrame == m_WireFrameEnabled)
      return;

    enableWireFrame ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_WireFrameEnabled = enableWireFrame;
  }

  void OpenGLRendererAPI::drawVertices(const VertexArray* vertexArray, uint32_t vertexCount)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(vertexArray, "Vertex array has not been initialized!");

    vertexArray->bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
  }

  void OpenGLRendererAPI::drawIndexed(const VertexArray* vertexArray, uint32_t indexCount)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(vertexArray, "Vertex array has not been initialized!");

    vertexArray->bind();
    uint32_t count = indexCount == 0 ? vertexArray->getIndexBuffer()->getCount() : indexCount;
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
  }

  void OpenGLRendererAPI::drawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(vertexArray, "Vertex array has not been initialized!");

    vertexArray->bind();
    uint32_t count = indexCount == 0 ? vertexArray->getIndexBuffer()->getCount() : indexCount;
    glDrawElements(GL_LINES, count, GL_UNSIGNED_INT, nullptr);
  }

  void OpenGLRendererAPI::multiDrawVertices(const void* drawCommands, int drawCount, int stride)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glMultiDrawArraysIndirect(GL_POINTS, drawCommands, static_cast<GLsizei>(drawCount), stride);
  }

  void OpenGLRendererAPI::clearDepthBuffer()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glClear(GL_DEPTH_BUFFER_BIT);
  }
}