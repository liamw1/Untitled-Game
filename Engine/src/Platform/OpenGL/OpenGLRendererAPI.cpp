#include "ENpch.h"
#include "OpenGLRendererAPI.h"
#include <glad/glad.h>

namespace Engine
{
  void OpenGLRendererAPI::initialize()
  {
    EN_PROFILE_FUNCTION();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
  }

  void OpenGLRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
  {
    EN_PROFILE_FUNCTION();

    glViewport(x, y, width, height);
  }

  void OpenGLRendererAPI::clear(const glm::vec4& color) const
  {
    EN_PROFILE_FUNCTION();

    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void OpenGLRendererAPI::drawIndexed(const Shared<VertexArray>& vertexArray)
  {
    glDrawElements(GL_TRIANGLES, vertexArray->getIndexBuffer()->getCount(), GL_UNSIGNED_INT, nullptr);
  }
}