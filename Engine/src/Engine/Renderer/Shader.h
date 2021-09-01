#pragma once
#include <string>

namespace Engine
{
  class Shader
  {
  public:
    Shader(const std::string& vertexSource, const std::string& fragmentSource);
    ~Shader();

    void bind() const;
    void unBind() const;

  private:
    uint32_t m_RendererID = 0;
  };
}