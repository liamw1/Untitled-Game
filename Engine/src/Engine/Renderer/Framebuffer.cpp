#include "ENpch.h"
#include "Framebuffer.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"

namespace Engine
{
  Unique<Framebuffer> Framebuffer::Create(const FramebufferSpecification& specification)
  {
    switch (Renderer::GetAPI())
    {
      case RendererAPI::API::None:    EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:  return CreateUnique<OpenGLFramebuffer>(specification);
      default:                        EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}
