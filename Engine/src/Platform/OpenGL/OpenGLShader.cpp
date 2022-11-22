#include "ENpch.h"
#include "OpenGLShader.h"
#include "Engine/Debug/Timer.h"
#include "Engine/Threading/Threads.h"
#include "Engine/Debug/Instrumentor.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace Engine
{
  static GLenum shaderTypeFromString(const std::string& type)
  {
    if (type == "vertex")
      return GL_VERTEX_SHADER;
    if (type == "fragment" || type == "pixel")
      return GL_FRAGMENT_SHADER;

    EN_CORE_ASSERT(false, "Unknown shader type {0}!", type);
    return 0;
  }

  static shaderc_shader_kind openGLShaderStageToShaderC(GLenum stage)
  {
    switch (stage)
    {
      case GL_VERTEX_SHADER:    return shaderc_glsl_vertex_shader;
      case GL_FRAGMENT_SHADER:  return shaderc_glsl_fragment_shader;
      default: EN_CORE_ERROR("Invalid openGL shader stage!");  return static_cast<shaderc_shader_kind>(0);
    }
  }

  static const char* openGLShaderStageToString(GLenum stage)
  {
    switch (stage)
    {
      case GL_VERTEX_SHADER:    return "GL_VERTEX_SHADER";
      case GL_FRAGMENT_SHADER:  return "GL_FRAGMENT_SHADER";
      default: EN_CORE_ERROR("Invalid openGL shader stage!");  return nullptr;
    }
  }

  static const char* getCacheDirectory()
  {
    // TODO: Make sure the assets directory is valid
    return "assets/cache/shader/opengl";
  }

  static void createCacheDirectoryIfNeeded()
  {
    std::string cacheDirectory = getCacheDirectory();
    if (!std::filesystem::exists(cacheDirectory))
      std::filesystem::create_directories(cacheDirectory);
  }

  static const char* openGLShaderStageCachedOpenGLFileExtension(uint32_t stage)
  {
    switch (stage)
    {
      case GL_VERTEX_SHADER:    return ".cached_opengl.vert";
      case GL_FRAGMENT_SHADER:  return ".cached_opengl.frag";
      default: EN_CORE_ERROR("Invalid openGL shader stage!");  return "";
    }
  }

  static const char* openGLShaderStageCachedVulkanFileExtension(uint32_t stage)
  {
    switch (stage)
    {
      case GL_VERTEX_SHADER:    return ".cached_vulkan.vert";
      case GL_FRAGMENT_SHADER:  return ".cached_vulkan.frag";
      default: EN_CORE_ERROR("Invalid openGL shader stage!");  return "";
    }
  }



  OpenGLShader::OpenGLShader(const std::string& filepath)
    : m_FilePath(filepath)
  {
    EN_PROFILE_FUNCTION();

    createCacheDirectoryIfNeeded();

    std::string source = readFile(filepath);
    std::unordered_map<uint32_t, std::string> shaderSources = preProcess(source);

    Timer timer("Shader creation");
    timer.timeStart();
    compileOrGetVulkanBinaries(shaderSources);
    compileOrGetOpenGLBinaries();
    createProgram();
    timer.timeStop();

    // Extract name from filepath
    size_t lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    size_t lastDot = filepath.rfind('.');
    size_t count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    m_Name = filepath.substr(lastSlash, count);
  }

  OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
    : m_Name(name)
  {
    EN_PROFILE_FUNCTION();

    std::unordered_map<GLenum, std::string> sources;
    sources[GL_VERTEX_SHADER] = vertexSource;
    sources[GL_FRAGMENT_SHADER] = fragmentSource;

    compileOrGetVulkanBinaries(sources);
    compileOrGetOpenGLBinaries();
    createProgram();
  }

  OpenGLShader::~OpenGLShader()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

    glDeleteProgram(m_RendererID);
  }

  void OpenGLShader::bind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    glUseProgram(m_RendererID);
  }

  void OpenGLShader::unBind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");
    glUseProgram(0);
  }

  std::string OpenGLShader::readFile(const std::string& filepath)
  {
    EN_PROFILE_FUNCTION();

    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);  // ifstream closes itself due to RAII
    if (in)
    {
      in.seekg(0, std::ios::end);
      size_t size = in.tellg();
      if (size != -1)
      {
        result.resize(size);
        in.seekg(0, std::ios::beg);
        in.read(&result[0], size);
      }
      else
        EN_CORE_ERROR("Could not read from file '{0}'", filepath);
    }
    else
      EN_CORE_ERROR("Could not open file {0}", filepath);

    return result;
  }

  std::unordered_map<GLenum, std::string> OpenGLShader::preProcess(const std::string& source)
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

  void OpenGLShader::compileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& shaderSources)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

    static constexpr bool optimize = true;

    GLuint program = glCreateProgram();

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    if (optimize)
      options.SetOptimizationLevel(shaderc_optimization_level_performance);

    std::filesystem::path cacheDirectory = getCacheDirectory();

    std::unordered_map<GLenum, std::vector<uint32_t>>& shaderData = m_VulkanSPIRV;
    shaderData.clear();
    for (auto&& [stage, source] : shaderSources)
    {
      std::filesystem::path shaderFilePath = m_FilePath;
      std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + openGLShaderStageCachedVulkanFileExtension(stage));

      std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
      if (in.is_open())
      {
        in.seekg(0, std::ios::end);
        std::streampos size = in.tellg();
        in.seekg(0, std::ios::beg);

        std::vector<uint32_t>& data = shaderData[stage];
        data.resize(size / sizeof(uint32_t));
        in.read(reinterpret_cast<char*>(data.data()), size);
      }
      else
      {
        shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, openGLShaderStageToShaderC(stage), m_FilePath.c_str(), options);
        if (module.GetCompilationStatus() != shaderc_compilation_status_success)
          EN_CORE_ERROR(module.GetErrorMessage());

        shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

        std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
        if (out.is_open())
        {
          std::vector<uint32_t>& data = shaderData[stage];
          out.write(reinterpret_cast<char*>(data.data()), sizeof(uint32_t) * data.size());
          out.flush();
          out.close();
        }
      }
    }

    for (auto&& [stage, data] : shaderData)
      reflect(stage, data);
  }

  void OpenGLShader::compileOrGetOpenGLBinaries()
  {
    static constexpr bool optimize = false;

    std::unordered_map<GLenum, std::vector<uint32_t>>& shaderData = m_OpenGLSPIRV;
    
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
    if (optimize)
      options.SetOptimizationLevel(shaderc_optimization_level_performance);

    std::filesystem::path cacheDirectory = getCacheDirectory();

    shaderData.clear();
    m_OpenGLSourceCode.clear();
    for (auto&& [stage, spirv] : m_VulkanSPIRV)
    {
      std::filesystem::path shaderFilePath = m_FilePath;
      std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + openGLShaderStageCachedOpenGLFileExtension(stage));

      std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
      if (in.is_open())
      {
        in.seekg(0, std::ios::end);
        std::streampos size = in.tellg();
        in.seekg(0, std::ios::beg);

        std::vector<uint32_t>& data = shaderData[stage];
        data.resize(size / sizeof(uint32_t));
        in.read(reinterpret_cast<char*>(data.data()), size);
      }
      else
      {
        spirv_cross::CompilerGLSL glslCompiler(spirv);
        m_OpenGLSourceCode[stage] = glslCompiler.compile();
        std::string& source = m_OpenGLSourceCode[stage];

        shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, openGLShaderStageToShaderC(stage), m_FilePath.c_str());
        if (module.GetCompilationStatus() != shaderc_compilation_status_success)
          EN_CORE_ERROR(module.GetErrorMessage());

        shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

        std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
        if (out.is_open())
        {
          std::vector<uint32_t>& data = shaderData[stage];
          out.write(reinterpret_cast<char*>(data.data()), sizeof(uint32_t) * data.size());
          out.flush();
          out.close();
        }
      }
    }
  }

  void OpenGLShader::createProgram()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

    GLuint program = glCreateProgram();

    std::vector<GLuint> shaderIDs;
    for (auto&& [stage, spirv] : m_OpenGLSPIRV)
    {
      GLuint shaderID = shaderIDs.emplace_back(glCreateShader(stage));
      glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), sizeof(uint32_t) * static_cast<GLsizei>(spirv.size()));
      glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
      glAttachShader(program, shaderID);
    }

    glLinkProgram(program);

    GLint isLinked;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
      GLint maxLength;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

      std::vector<GLchar> infoLog(maxLength);
      glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
      EN_CORE_ERROR("Shader linking failed ({0}):\n{1}", m_FilePath, infoLog.data());

      glDeleteProgram(program);

      for (uint32_t id : shaderIDs)
        glDeleteShader(id);
    }

    for (uint32_t id : shaderIDs)
    {
      glDetachShader(program, id);
      glDeleteShader(id);
    }

    m_RendererID = program;
  }

  void OpenGLShader::reflect(GLenum stage, const std::vector<uint32_t>& shaderData)
  {
    spirv_cross::Compiler compiler(shaderData);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    EN_CORE_TRACE("OpenGLShader::Reflect - {0} {1}", openGLShaderStageToString(stage), m_FilePath);
    EN_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
    EN_CORE_TRACE("    {0} resources", resources.sampled_images.size());

    EN_CORE_TRACE("Uniform buffers:");
    for (const spirv_cross::Resource& uniform : resources.uniform_buffers)
    {
      const spirv_cross::SPIRType& bufferType = compiler.get_type(uniform.base_type_id);
      size_t bufferSize = compiler.get_declared_struct_size(bufferType);
      uint32_t binding = compiler.get_decoration(uniform.id, spv::DecorationBinding);
      int memberCount = static_cast<int>(bufferType.member_types.size());

      EN_CORE_TRACE("  {0}", uniform.name);
      EN_CORE_TRACE("    Size = {0}", bufferSize);
      EN_CORE_TRACE("    Binding = {0}", binding);
      EN_CORE_TRACE("    Members = {0}", memberCount);
    }
  }

  GLint OpenGLShader::getUniformLocation(const std::string& name) const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::GetMainThreadID(), "OpenGL calls must be made in main thread!");

    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
      return m_UniformLocationCache[name];

    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    m_UniformLocationCache[name] = location;
    return location;
  }
}