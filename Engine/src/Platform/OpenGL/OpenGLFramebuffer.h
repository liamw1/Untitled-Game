#pragma once
#include "Engine/Renderer/Framebuffer.h"

namespace Engine
{
  class OpenGLFramebuffer : public Framebuffer
  {
  public:
    OpenGLFramebuffer(const FramebufferSpecification& specification);
    ~OpenGLFramebuffer();

    void bind() override;
    void unbind() override;

    const FramebufferSpecification& getSpecialization() const override { return m_Specification; }
    uint32_t getColorAttachmentRendererID() const { return m_ColorAttachment; }

    void invalidate();

  private:
    uint32_t m_RendererID;
    uint32_t m_ColorAttachment;
    uint32_t m_DepthAttachment;
    FramebufferSpecification m_Specification;
  };
}