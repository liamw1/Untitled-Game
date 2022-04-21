#include "ENpch.h"
#include "OpenGLRendererAPI.h"
#include "Engine/Threading/Threads.h"
#include "Engine/Debug/Instrumentor.h"
#include <glad/glad.h>

namespace Engine
{
  void OpenGLRendererAPI::initialize()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // glEnable(GL_MULTISAMPLE);
  }

  void OpenGLRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    glViewport(x, y, width, height);
  }

  void OpenGLRendererAPI::clear(const Float4& color)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void OpenGLRendererAPI::wireFrameToggle(bool enableWireFrame)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    enableWireFrame ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  void OpenGLRendererAPI::faceCullToggle(bool enableFaceCulling)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    enableFaceCulling ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
  }

  void OpenGLRendererAPI::drawVertices(const VertexArray* vertexArray, uint32_t vertexCount)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_ASSERT(vertexArray != nullptr, "Vertex array has not been initialized!");

    vertexArray->bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
  }

  void OpenGLRendererAPI::drawIndexed(const VertexArray* vertexArray, uint32_t indexCount)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_ASSERT(vertexArray != nullptr, "Vertex array has not been initialized!");

    vertexArray->bind();
    uint32_t count = indexCount == 0 ? vertexArray->getIndexBuffer()->getCount() : indexCount;
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
  }

  void OpenGLRendererAPI::drawIndexedLines(const VertexArray* vertexArray, uint32_t indexCount)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    EN_ASSERT(vertexArray != nullptr, "Vertex array has not been initialized!");

    vertexArray->bind();
    uint32_t count = indexCount == 0 ? vertexArray->getIndexBuffer()->getCount() : indexCount;
    glDrawElements(GL_LINES, count, GL_UNSIGNED_INT, nullptr);
  }

  void OpenGLRendererAPI::clearDepthBuffer()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    glClear(GL_DEPTH_BUFFER_BIT);
  }
}