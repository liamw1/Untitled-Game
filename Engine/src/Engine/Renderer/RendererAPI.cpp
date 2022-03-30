#include "ENpch.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Engine
{
  RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;
}