#include "ENpch.h"
#include "OpenGL_LegacyShader.h"
#include "Engine/Threading/Threads.h"
#include "Engine/Debug/Instrumentor.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Engine
{
  static constexpr int c_MaxShaders = 3;

  static GLenum shaderTypeFromString(const std::string& type)
  {
    if (type == "vertex")
      return GL_VERTEX_SHADER;
    if (type == "geometry")
      return GL_GEOMETRY_SHADER;
    if (type == "fragment" || type == "pixel")
      return GL_FRAGMENT_SHADER;

    EN_CORE_ASSERT(false, "Unknown shader type {0}!", type);
    return 0;
  }



  OpenGL_LegacyShader::OpenGL_LegacyShader(const std::string& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
    : m_RendererID(0),
      m_Name("Unnamed Shader")
  {
    EN_PROFILE_FUNCTION();

    compile(PreProcess(ReadFile(filepath), preprocessorDefinitions));

    std::filesystem::path path = filepath;
    m_Name = path.stem().string(); // Returns the file's name stripped of the extension.
  }

  OpenGL_LegacyShader::~OpenGL_LegacyShader()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glDeleteProgram(m_RendererID);
  }

  const std::string& OpenGL_LegacyShader::name() const
  {
    return m_Name;
  }

  void OpenGL_LegacyShader::bind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glUseProgram(m_RendererID);
  }

  void OpenGL_LegacyShader::unBind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glUseProgram(0);
  }

  void OpenGL_LegacyShader::compile(const std::unordered_map<std::string, std::string>& shaderSources)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    EN_CORE_ASSERT(shaderSources.size() <= c_MaxShaders, "A maximum of {0} shaders is supported", c_MaxShaders);

    GLuint program = glCreateProgram();
    std::vector<GLenum> glShaderIDs(shaderSources.size());
    int glShaderIDIndex = 0;
    for (const auto& [type, source] : shaderSources)
    {
      GLuint shader = glCreateShader(shaderTypeFromString(type));

      const GLchar* sourceCStr = source.c_str();
      glShaderSource(shader, 1, &sourceCStr, 0);

      glCompileShader(shader);

      GLint isCompiled = 0;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
      if (!isCompiled)
      {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

        glDeleteShader(shader);

        EN_CORE_ERROR("{0}", infoLog.data());
        EN_CORE_ASSERT(false, "{0} shader compilation failure!", type);
        break;
      }
      glAttachShader(program, shader);
      glShaderIDs[glShaderIDIndex++] = shader;
    }

    m_RendererID = program;
    glLinkProgram(m_RendererID);

    GLint isLinked = 0;
    glGetProgramiv(m_RendererID, GL_LINK_STATUS, (int*)&isLinked);
    if (!isLinked)
    {
      GLint maxLength = 0;
      glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLength);

      // The maxLength includes the NULL character
      std::vector<GLchar> infoLog(maxLength);
      glGetProgramInfoLog(m_RendererID, maxLength, &maxLength, &infoLog[0]);

      glDeleteProgram(m_RendererID);

      for (GLenum id : glShaderIDs)
      {
        glDetachShader(m_RendererID, id);
        glDeleteShader(id);
      }

      EN_CORE_ERROR("{0}", infoLog.data());
      EN_CORE_ASSERT(false, "Shader link failure!");
      return;
    }

    // Always detach shaders after a successful link
    for (GLenum id : glShaderIDs)
      glDetachShader(m_RendererID, id);
  }
}