#include "ENpch.h"
#include "OpenGLShader.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Engine
{
  static GLenum shaderTypeFromString(const std::string& type)
  {
    if (type == "vertex")
      return GL_VERTEX_SHADER;
    if (type == "fragment" || type == "pixel")
      return GL_FRAGMENT_SHADER;

    EN_CORE_ASSERT(false, "Unknown shader type '{0}'!", type);
    return 0;
  }

  OpenGLShader::OpenGLShader(const std::string& filepath)
  {
    compile(preProcess(readFile(filepath)));
  }

  OpenGLShader::OpenGLShader(const std::string& vertexSource, const std::string& fragmentSource)
  {
    std::unordered_map<GLenum, std::string> sources;
    sources[GL_VERTEX_SHADER] = vertexSource;
    sources[GL_FRAGMENT_SHADER] = fragmentSource;
    compile(sources);
  }

  OpenGLShader::~OpenGLShader()
  {
    glDeleteProgram(m_RendererID);
  }

  void OpenGLShader::bind() const
  {
    glUseProgram(m_RendererID);
  }

  void OpenGLShader::unBind() const
  {
    glUseProgram(0);
  }

  void OpenGLShader::uploadUniformInt(const std::string& name, int value)
  {
    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    glUniform1i(location, value);
  }

  void OpenGLShader::uploadUniformFloat(const std::string& name, const float value)
  {
    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    glUniform1f(location, value);
  }

  void OpenGLShader::uploadUniformFloat2(const std::string& name, const glm::vec2& values)
  {
    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    glUniform2f(location, values.x, values.y);
  }

  void OpenGLShader::uploadUniformFloat3(const std::string& name, const glm::vec3& values)
  {
    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    glUniform3f(location, values.x, values.y, values.z);
  }

  void OpenGLShader::uploadUniformFloat4(const std::string& name, const glm::vec4& values)
  {
    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    glUniform4f(location, values.x, values.y, values.z, values.w);
  }

  void OpenGLShader::uploadUniformMat3(const std::string& name, const glm::mat3& matrix)
  {
    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
  }

  void OpenGLShader::uploadUniformMat4(const std::string& name, const glm::mat4& matrix)
  {
    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
  }

  std::string OpenGLShader::readFile(const std::string& filepath)
  {
    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);
    if (in)
    {
      in.seekg(0, std::ios::end);
      result.resize(in.tellg());
      in.seekg(0, std::ios::beg);
      in.read(&result[0], result.size());
      in.close();
    }
    else
      EN_CORE_ERROR("Could not open file '{0}'", filepath);

    return result;
  }

  std::unordered_map<GLenum, std::string> OpenGLShader::preProcess(const std::string& source)
  {
    std::unordered_map<GLenum, std::string> shaderSources;

    const char* typeToken = "#type";
    size_t typeTokenLength = strlen(typeToken);
    size_t pos = source.find(typeToken, 0);
    while (pos != std::string::npos)
    {
      size_t eol = source.find_first_of("\r\n", pos);
      EN_CORE_ASSERT(eol != std::string::npos, "Syntax error");
      size_t begin = pos + typeTokenLength + 1;
      std::string type = source.substr(begin, eol - begin);

      size_t nextLinePos = source.find_first_not_of("\r\n", eol);
      EN_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
      pos = source.find(typeToken, nextLinePos);
      size_t count = pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos);
      shaderSources[shaderTypeFromString(type)] = source.substr(nextLinePos, count);
    }

    return shaderSources;
  }

  void OpenGLShader::compile(const std::unordered_map<GLenum, std::string>& shaderSources)
  {
    GLuint program = glCreateProgram();
    std::vector<GLenum> glShaderIDs;
    glShaderIDs.reserve(shaderSources.size());
    for (auto& kv : shaderSources)
    {
      GLenum type = kv.first;
      const std::string& source = kv.second;

      GLuint shader = glCreateShader(type);

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
        EN_CORE_ASSERT(false, "'{0}' shader compilation failure!", type);
        break;
      }
      glAttachShader(program, shader);
      glShaderIDs.emplace_back(shader);
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

      for (auto id : glShaderIDs)
        glDeleteShader(id);

      EN_CORE_ERROR("{0}", infoLog.data());
      EN_CORE_ASSERT(false, "Shader link failure!");
      return;
    }

    // Always detach shaders after a successful link
    for (auto id : glShaderIDs)
      glDetachShader(m_RendererID, id);
  }
}