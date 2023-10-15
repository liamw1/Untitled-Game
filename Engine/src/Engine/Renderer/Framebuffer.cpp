#include "ENpch.h"
#include "Framebuffer.h"
#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"

namespace eng
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
      case RendererAPI::API::OpenGL:        return std::make_unique<OpenGLFramebuffer>(specification);
      case RendererAPI::API::OpenGL_Legacy: return std::make_unique<OpenGLFramebuffer>(specification);
      default:                              throw  std::invalid_argument("Unknown RendererAPI!");
    }
  }
}
