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

namespace eng
{
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

  static shaderc_shader_kind openGLShaderStageToShaderC(GLenum stage)
  {
    switch (stage)
    {
      case GL_VERTEX_SHADER:    return shaderc_glsl_vertex_shader;
      case GL_GEOMETRY_SHADER:  return shaderc_glsl_geometry_shader;
      case GL_FRAGMENT_SHADER:  return shaderc_glsl_fragment_shader;
      default: ENG_CORE_ERROR("Invalid openGL shader stage!");  return static_cast<shaderc_shader_kind>(0);
    }
  }

  static const char* openGLShaderStageToString(GLenum stage)
  {
    switch (stage)
    {
      case GL_VERTEX_SHADER:    return "GL_VERTEX_SHADER";
      case GL_GEOMETRY_SHADER:  return "GL_GEOMETRY_SHADER";
      case GL_FRAGMENT_SHADER:  return "GL_FRAGMENT_SHADER";
      default: ENG_CORE_ERROR("Invalid openGL shader stage!");  return nullptr;
    }
  }



  OpenGLShader::OpenGLShader(const std::string& filepath, const std::unordered_map<std::string, std::string>& preprocessorDefinitions)
    : m_RendererID(0),
      m_Name("Unnamed Shader"),
      m_FilePath(filepath)
  {
    ENG_PROFILE_FUNCTION();

    std::string source = ReadFile(filepath);
    std::unordered_map<std::string, std::string> shaderSources = PreProcess(source, preprocessorDefinitions);

    debug::Timer timer("Shader creation");
    timer.timeStart();
    compileVulkanBinaries(shaderSources);
    compileOpenGLBinaries();
    createProgram();
    timer.timeStop();

    // Extract name from filepath
    uSize lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    uSize lastDot = filepath.rfind('.');
    uSize count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    m_Name = filepath.substr(lastSlash, count);
  }

  OpenGLShader::~OpenGLShader()
  {
    ENG_PROFILE_FUNCTION();
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    glDeleteProgram(m_RendererID);
  }

  const std::string& OpenGLShader::name() const
  {
    return m_Name;
  }

  void OpenGLShader::bind() const
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glUseProgram(m_RendererID);
  }

  void OpenGLShader::unBind() const
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");
    glUseProgram(0);
  }

  void OpenGLShader::compileVulkanBinaries(const std::unordered_map<std::string, std::string>& shaderSources)
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    static constexpr bool optimize = true;

    GLuint program = glCreateProgram();

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    if (optimize)
      options.SetOptimizationLevel(shaderc_optimization_level_performance);

    std::unordered_map<GLenum, std::vector<u32>>& shaderData = m_VulkanSPIRV;
    shaderData.clear();
    for (const auto& [type, source] : shaderSources)
    {
      GLenum stage = shaderTypeFromString(type);

      shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, openGLShaderStageToShaderC(stage), m_FilePath.c_str(), options);
      if (module.GetCompilationStatus() != shaderc_compilation_status_success)
        ENG_CORE_ERROR(module.GetErrorMessage());

      shaderData[stage] = std::vector<u32>(module.cbegin(), module.cend());
    }

    for (const auto& [stage, data] : shaderData)
      reflect(stage, data);
  }

  void OpenGLShader::compileOpenGLBinaries()
  {
    static constexpr bool optimize = false;

    std::unordered_map<GLenum, std::vector<u32>>& shaderData = m_OpenGLSPIRV;
    
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
        ENG_CORE_ERROR(module.GetErrorMessage());

      shaderData[stage] = std::vector<u32>(module.cbegin(), module.cend());
    }
  }

  void OpenGLShader::createProgram()
  {
    ENG_CORE_ASSERT(threads::isMainThread(), "OpenGL calls must be made on the main thread!");

    GLuint program = glCreateProgram();

    std::vector<GLuint> shaderIDs;
    for (auto&& [stage, spirv] : m_OpenGLSPIRV)
    {
      GLuint shaderID = shaderIDs.emplace_back(glCreateShader(stage));
      glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), sizeof(u32) * static_cast<GLsizei>(spirv.size()));
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
      ENG_CORE_ERROR("Shader linking failed ({0}):\n{1}", m_FilePath, infoLog.data());

      glDeleteProgram(program);

      for (u32 id : shaderIDs)
        glDeleteShader(id);
    }

    for (u32 id : shaderIDs)
    {
      glDetachShader(program, id);
      glDeleteShader(id);
    }

    m_RendererID = program;
  }

  void OpenGLShader::reflect(u32 stage, const std::vector<u32>& shaderData)
  {
    spirv_cross::Compiler compiler(shaderData);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    ENG_CORE_TRACE("OpenGLShader::Reflect - {0} {1}", openGLShaderStageToString(stage), m_FilePath);
    ENG_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
    ENG_CORE_TRACE("    {0} resources", resources.sampled_images.size());

    ENG_CORE_TRACE("Uniform buffers:");
    for (const spirv_cross::Resource& uniform : resources.uniform_buffers)
    {
      const spirv_cross::SPIRType& bufferType = compiler.get_type(uniform.base_type_id);
      uSize bufferSize = compiler.get_declared_struct_size(bufferType);
      u32 binding = compiler.get_decoration(uniform.id, spv::DecorationBinding);
      i32 memberCount = static_cast<i32>(bufferType.member_types.size());

      ENG_CORE_TRACE("  {0}", uniform.name);
      ENG_CORE_TRACE("    Size = {0}", bufferSize);
      ENG_CORE_TRACE("    Binding = {0}", binding);
      ENG_CORE_TRACE("    Members = {0}", memberCount);
    }
  }
}