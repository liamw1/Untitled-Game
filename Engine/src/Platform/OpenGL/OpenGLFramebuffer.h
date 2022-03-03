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

    void resize(uint32_t width, uint32_t height) override;

  private:
    uint32_t m_RendererID = 0;
    uint32_t m_ColorAttachment = 0;
    uint32_t m_DepthAttachment = 0;
    FramebufferSpecification m_Specification;
  };
}