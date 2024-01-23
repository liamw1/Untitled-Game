#include "ENpch.h"
#include "OpenGL_LegacyShader.h"
#include "Engine/Core/Exception.h"
#include "Engine/Threads/Threads.h"
#include "Engine/Debug/Assert.h"
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



  OpenGL_LegacyShader::OpenGL_LegacyShader(const std::filesystem::path& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
    : m_ShaderID(0),
      m_Name(filepath.stem().string())
  {
    ENG_PROFILE_FUNCTION();
    compile(PreProcess(ReadFile(filepath), preprocessorDefinitions));
  }

  OpenGL_LegacyShader::~OpenGL_LegacyShader()
  {
    ENG_PROFILE_FUNCTION();
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");

    glDeleteProgram(m_ShaderID);
  }

  const std::string& OpenGL_LegacyShader::name() const
  {
    return m_Name;
  }

  void OpenGL_LegacyShader::bind() const
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glUseProgram(m_ShaderID);
  }

  void OpenGL_LegacyShader::unbind() const
  {
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    glUseProgram(0);
  }

  void OpenGL_LegacyShader::compile(const std::unordered_map<std::string, std::string>& shaderSources)
  {
    ENG_PROFILE_FUNCTION();
    ENG_CORE_ASSERT(thread::isMainThread(), "OpenGL calls must be made on the main thread!");
    ENG_CORE_ASSERT(withinBounds(shaderSources.size(), 0, c_MaxShaders + 1), "A maximum of {0} shaders is supported", c_MaxShaders);

    GLuint programID = glCreateProgram();

    std::vector<GLuint> glShaderIDs;
    for (const auto& [type, source] : shaderSources)
    {
      GLuint shaderID = glCreateShader(shaderTypeFromString(type));

      const GLchar* sourceCStr = source.c_str();
      glShaderSource(shaderID, 1, &sourceCStr, 0);
      glCompileShader(shaderID);

      GLint isCompiled = 0;
      glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
      if (!isCompiled)
      {
        GLint maxLength = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(shaderID, maxLength, &maxLength, infoLog.data());

        glDeleteShader(shaderID);
        throw CoreException(std::string("Shader compilation failure!\n") + infoLog.data());
      }
      glAttachShader(programID, shaderID);
      glShaderIDs.push_back(shaderID);
    }

    glLinkProgram(programID);

    GLint isLinked = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &isLinked);
    if (!isLinked)
    {
      GLint maxLength = 0;
      glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);

      std::vector<GLchar> infoLog(maxLength);
      glGetProgramInfoLog(programID, maxLength, &maxLength, infoLog.data());

      glDeleteProgram(programID);
      for (GLuint id : glShaderIDs)
      {
        glDetachShader(programID, id);
        glDeleteShader(id);
      }
      throw CoreException(std::string("Shader link failure!\n") + infoLog.data());
    }

    // Always detach shaders after a successful link
    for (GLuint id : glShaderIDs)
      glDetachShader(programID, id);

    m_ShaderID = programID;
  }
}