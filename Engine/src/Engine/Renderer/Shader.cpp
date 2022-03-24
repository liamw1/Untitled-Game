#include "ENpch.h"
#include "Shader.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace Engine
{
  Shared<Shader> Shader::Create(const std::string& filepath)
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return CreateShared<OpenGLShader>(filepath);
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
  Shared<Shader> Shader::Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return CreateShared<OpenGLShader>(name, vertexSource, fragmentSource);
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }



  void ShaderLibrary::add(const Shared<Shader>& shader)
  {
    add(shader->getName(), shader);
  }

  void ShaderLibrary::add(const std::string& name, const Shared<Shader>& shader)
  {
    EN_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end(), "Shader already exists!");
    m_Shaders[name] = shader;
  }

  Shared<Shader> ShaderLibrary::load(const std::string& filepath)
  {
    auto shader = Shader::Create(filepath);
    add(shader);
    return shader;
  }

  Shared<Shader> ShaderLibrary::load(const std::string& name, const std::string& filepath)
  {
    auto shader = Shader::Create(filepath);
    add(name, shader);
    return shader;
  }

  Shared<Shader> ShaderLibrary::get(const std::string& name)
  {
    EN_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end(), "Shader not found!");
    return m_Shaders[name];
  }
}