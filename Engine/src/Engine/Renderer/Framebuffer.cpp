#include "ENpch.h"
#include "Framebuffer.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"

namespace Engine
{
  FramebufferTextureSpecification::FramebufferTextureSpecification()
    : textureFormat(FramebufferTextureFormat::None) {}
  FramebufferTextureSpecification::FramebufferTextureSpecification(FramebufferTextureFormat format)
    : textureFormat(format) {}

  FramebufferSpecification::FramebufferSpecification()
    : width(0),
      height(0),
      samples(1),
      swapChainTarget(false) {}
  FramebufferSpecification::FramebufferSpecification(const std::initializer_list<FramebufferTextureSpecification>& attachmentList)
    : FramebufferSpecification()
  {
    attachments = attachmentList;
  }

  std::unique_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& specification)
  {
    switch (RendererAPI::GetAPI())
    {
      case RendererAPI::API::None:          EN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLFramebuffer>(specification);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLFramebuffer>(specification);
      default:                              EN_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
    }
  }
}
