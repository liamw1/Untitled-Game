#include "ENpch.h"
#include "OpenGLShader.h"
#include "Engine/Debug/Timer.h"
#include "Engine/Threads/Threads.h"
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
    if (type == "geometry")
      return GL_GEOMETRY_SHADER;
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
      case GL_GEOMETRY_SHADER:  return shaderc_glsl_geometry_shader;
      case GL_FRAGMENT_SHADER:  return shaderc_glsl_fragment_shader;
      default: EN_CORE_ERROR("Invalid openGL shader stage!");  return static_cast<shaderc_shader_kind>(0);
    }
  }

  static const char* openGLShaderStageToString(GLenum stage)
  {
    switch (stage)
    {
      case GL_VERTEX_SHADER:    return "GL_VERTEX_SHADER";
      case GL_GEOMETRY_SHADER:  return "GL_GEOMETRY_SHADER";
      case GL_FRAGMENT_SHADER:  return "GL_FRAGMENT_SHADER";
      default: EN_CORE_ERROR("Invalid openGL shader stage!");  return nullptr;
    }
  }



  OpenGLShader::OpenGLShader(const std::string& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
    : m_RendererID(0),
      m_Name("Unnamed Shader"),
      m_FilePath(filepath)
  {
    EN_PROFILE_FUNCTION();

    std::string source = ReadFile(filepath);
    std::unordered_map<std::string, std::string> shaderSources = PreProcess(source, preprocessorDefinitions);

    Debug::Timer timer("Shader creation");
    timer.timeStart();
    compileVulkanBinaries(shaderSources);
    compileOpenGLBinaries();
    createProgram();
    timer.timeStop();

    // Extract name from filepath
    size_t lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    size_t lastDot = filepath.rfind('.');
    size_t count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    m_Name = filepath.substr(lastSlash, count);
  }

  OpenGLShader::~OpenGLShader()
  {
    EN_PROFILE_FUNCTION();
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    glDeleteProgram(m_RendererID);
  }

  const std::string& OpenGLShader::name() const
  {
    return m_Name;
  }

  void OpenGLShader::bind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glUseProgram(m_RendererID);
  }

  void OpenGLShader::unBind() const
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");
    glUseProgram(0);
  }

  void OpenGLShader::compileVulkanBinaries(const std::unordered_map<std::string, std::string>& shaderSources)
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

    static constexpr bool optimize = true;

    GLuint program = glCreateProgram();

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    if (optimize)
      options.SetOptimizationLevel(shaderc_optimization_level_performance);

    std::unordered_map<GLenum, std::vector<uint32_t>>& shaderData = m_VulkanSPIRV;
    shaderData.clear();
    for (const auto& [type, source] : shaderSources)
    {
      GLenum stage = shaderTypeFromString(type);

      shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, openGLShaderStageToShaderC(stage), m_FilePath.c_str(), options);
      if (module.GetCompilationStatus() != shaderc_compilation_status_success)
        EN_CORE_ERROR(module.GetErrorMessage());

      shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
    }

    for (const auto& [stage, data] : shaderData)
      reflect(stage, data);
  }

  void OpenGLShader::compileOpenGLBinaries()
  {
    static constexpr bool optimize = false;

    std::unordered_map<GLenum, std::vector<uint32_t>>& shaderData = m_OpenGLSPIRV;
    
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
    if (optimize)
      options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderData.clear();
    m_OpenGLSourceCode.clear();
    for (auto&& [stage, spirv] : m_VulkanSPIRV)
    {
      spirv_cross::CompilerGLSL glslCompiler(spirv);
      m_OpenGLSourceCode[stage] = glslCompiler.compile();
      std::string& source = m_OpenGLSourceCode[stage];

      shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, openGLShaderStageToShaderC(stage), m_FilePath.c_str());
      if (module.GetCompilationStatus() != shaderc_compilation_status_success)
        EN_CORE_ERROR(module.GetErrorMessage());

      shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
    }
  }

  void OpenGLShader::createProgram()
  {
    EN_CORE_ASSERT(std::this_thread::get_id() == Threads::MainThreadID(), "OpenGL calls must be made on the main thread!");

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

  void OpenGLShader::reflect(uint32_t stage, const std::vector<uint32_t>& shaderData)
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
}