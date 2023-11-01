#include "ENpch.h"
#include "OpenGL_LegacyShader.h"
#include "Engine/Threads/Threads.h"
#include "Engine/Debug/Instrumentor.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace eng
{
  static constexpr i32 c_MaxShaders = 3;

  static GLenum shaderTypeFromString(const std::string& type)
  {
    if (type == "vertex")
      return GL_VERTEX_SHADER;
    if (type == "geometry")
      return GL_GEOMETRY_SHADER;
    if (type == "fragment" || type == "pixel")
      return GL_FRAGMENT_SHADER;

    ENG_CORE_ASSERT(false, "Unknown shader type {0}!", type);
    return 0;
  }



  OpenGL_LegacyShader::OpenGL_LegacyShader(const std::string& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
    : m_RendererID(0),
      m_Name("Unnamed Shader")
  {
    ENG_PROFILE_FUNCTION();

    compile(PreProcess(ReadFile(filepath), preprocessorDefinitions));

    std::filesystem::path path = filepath;
    m_Name = path.stem().string(); // Returns the file's name stripped of the extension.
  }

  OpenGL_LegacyShader::~OpenGL_LegacyShader()
  {
    ENG_PROFILE_FUNCTION();
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glDeleteProgram(m_RendererID);
  }

  const std::string& OpenGL_LegacyShader::name() const
  {
    return m_Name;
  }

  void OpenGL_LegacyShader::bind() const
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glUseProgram(m_RendererID);
  }

  void OpenGL_LegacyShader::unBind() const
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glUseProgram(0);
  }

  void OpenGL_LegacyShader::compile(const std::unordered_map<std::string, std::string>& shaderSources)
  {
    ENG_PROFILE_FUNCTION();
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(debug::boundsCheck(shaderSources.size(), 0, c_MaxShaders + 1), "A maximum of {0} shaders is supported", c_MaxShaders);

    GLuint program = glCreateProgram();
    std::vector<GLenum> glShaderIDs(shaderSources.size());
    i32 glShaderIDIndex = 0;
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

        ENG_CORE_ERROR("{0}", infoLog.data());
        ENG_CORE_ASSERT(false, "{0} shader compilation failure!", type);
        break;
      }
      glAttachShader(program, shader);
      glShaderIDs[glShaderIDIndex++] = shader;
    }

    m_RendererID = program;
    glLinkProgram(m_RendererID);

    GLint isLinked = 0;
    glGetProgramiv(m_RendererID, GL_LINK_STATUS, &isLinked);
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

      ENG_CORE_ERROR("{0}", infoLog.data());
      ENG_CORE_ASSERT(false, "Shader link failure!");
      return;
    }

    // Always detach shaders after a successful link
    for (GLenum id : glShaderIDs)
      glDetachShader(m_RendererID, id);
  }
}