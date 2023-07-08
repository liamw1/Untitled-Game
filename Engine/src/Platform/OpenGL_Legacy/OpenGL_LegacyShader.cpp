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

  static std::string readFile(const std::string& filepath)
  {
    EN_PROFILE_FUNCTION();

    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);  // ifstream closes itself due to RAII
    if (in)
    {
      const size_t& size = in.tellg();
      if (size != -1)
      {
        in.seekg(0, std::ios::end);
        result.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&result[0], result.size());
      }
      else
        EN_CORE_ERROR("Could not read from file {0}", filepath);
    }
    else
      EN_CORE_ERROR("Could not open file {0}", filepath);

    return result;
  }

  static std::unordered_map<GLenum, std::string> preProcess(const std::string& source)
  {
    EN_PROFILE_FUNCTION();

    std::unordered_map<GLenum, std::string> shaderSources;

    const char* typeToken = "#type";
    size_t typeTokenLength = strlen(typeToken);
    size_t pos = source.find(typeToken, 0);             // Start of shader type declaration line
    while (pos != std::string::npos)
    {
      size_t eol = source.find_first_of("\r\n", pos);   // End of shader type declaration line
      EN_CORE_ASSERT(eol != std::string::npos, "Syntax error");
      size_t begin = pos + typeTokenLength + 1;         // Start of shader type name (after "#type" keyword)
      std::string type = source.substr(begin, eol - begin);

      size_t nextLinePos = source.find_first_not_of("\r\n", eol);
      EN_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
      pos = source.find(typeToken, nextLinePos);        // Start of next shader type declaration line
      shaderSources[shaderTypeFromString(type)] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
    }

    return shaderSources;
  }



  OpenGL_LegacyShader::OpenGL_LegacyShader(const std::string& filepath)
    : m_UniformLocationCache({})
  {
    EN_PROFILE_FUNCTION();

    compile(preProcess(readFile(filepath)));

    std::filesystem::path path = filepath;
    m_Name = path.stem().string(); // Returns the file's name stripped of the extension.
  }

  OpenGL_LegacyShader::~OpenGL_LegacyShader()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glDeleteProgram(m_RendererID);
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

  void OpenGL_LegacyShader::compile(const std::unordered_map<GLenum, std::string>& shaderSources)
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    EN_CORE_ASSERT(shaderSources.size() <= c_MaxShaders, "A maximum of {0} shaders is supported", c_MaxShaders);

    GLuint program = glCreateProgram();
    std::vector<GLenum> glShaderIDs(shaderSources.size());
    int glShaderIDIndex = 0;
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

  GLint OpenGL_LegacyShader::getUniformLocation(const std::string& name) const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
      return m_UniformLocationCache[name];

    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    m_UniformLocationCache[name] = location;
    return location;
  }
}