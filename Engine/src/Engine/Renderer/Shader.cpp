#include "ENpch.h"
#include "Shader.h"

#include <glad/glad.h>

namespace Engine
{
  Shader::Shader(const std::string& vertexSource, const std::string& fragmentSource)
  {
    // Create an empty vertex shader handle
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // Send the vertex shader source to GL
    // Note that std::string's .c_str is NULL character terminated
    const GLchar* source = vertexSource.c_str();
    glShaderSource(vertexShader, 1, &source, 0);

    glCompileShader(vertexShader);

    GLint isCompiled = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
    if (!isCompiled)
    {
      GLint maxLength = 0;
      glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

      // The maxLength includes the NULL character
      std::vector<GLchar> infoLog(maxLength);
      glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);

      glDeleteShader(vertexShader);
      
      EN_CORE_ERROR("{0}", infoLog.data());
      EN_CORE_ASSERT(false, "Vertex shader compilation failure!");
      return;
    }

    // Create an empty fragment shader handle
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    // Send the fragment shader source code to GL
    // Note that std::string's .c_str is NULL character terminated
    source = fragmentSource.c_str();
    glShaderSource(fragmentShader, 1, &source, 0);

    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
    if (!isCompiled)
    {
      GLint maxLength = 0;
      glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

      // The maxLength includes the NULL character
      std::vector<GLchar> infoLog(maxLength);
      glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);

      glDeleteShader(fragmentShader);
      glDeleteShader(vertexShader);

      EN_CORE_ERROR("{0}", infoLog.data());
      EN_CORE_ASSERT(false, "Fragment shader compilation failure!");
      return;
    }

    // Vertex and fragment shaders are sucessfully compiled
    // Now time to link them together into a program
    m_RendererID = glCreateProgram();

    // Attach shaders to program
    glAttachShader(m_RendererID, vertexShader);
    glAttachShader(m_RendererID, fragmentShader);

    glLinkProgram(m_RendererID);

    // Note the different functions here: glGetProgram* instead of glGetShader*
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
      glDeleteShader(vertexShader);
      glDeleteShader(fragmentShader);

      EN_CORE_ERROR("{0}", infoLog.data());
      EN_CORE_ASSERT(false, "Shader link failure!");
      return;
    }

    // Always detach shaders after a successful link
    glDetachShader(m_RendererID, vertexShader);
    glDetachShader(m_RendererID, fragmentShader);
  }

  Shader::~Shader()
  {
    glDeleteProgram(m_RendererID);
  }

  void Shader::bind() const
  {
    glUseProgram(m_RendererID);
  }

  void Shader::unBind() const
  {
    glUseProgram(0);
  }
}