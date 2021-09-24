#include "ENpch.h"
#include "RenderCommand.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Engine
{
  Unique<RendererAPI> RenderCommand::s_RendererAPI = CreateUnique<OpenGLRendererAPI>();
}