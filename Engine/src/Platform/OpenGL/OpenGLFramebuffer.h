#pragma once
#include "Engine/Renderer/Framebuffer.h"

namespace eng
{
  class OpenGLFramebuffer : public Framebuffer
  {
    u32 m_RendererID;
    FramebufferSpecification m_Specification;

    std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
    FramebufferTextureSpecification m_DepthAttachmentSpecification;

    std::vector<u32> m_ColorAttachments;
    u32 m_DepthAttachment;

  public:
    OpenGLFramebuffer(const FramebufferSpecification& specification);
    ~OpenGLFramebuffer();

    void bind() override;
    void unbind() override;

    const FramebufferSpecification& specification() const override;
    u32 getColorAttachmentRendererID(u32 index = 0) const override;

    virtual void copyToWindow() const override;

    void resize(u32 width, u32 height) override;
    i32 readPixel(u32 attachmentIndex, i32 x, i32 y) override;

    void clearAttachment(u32 attachmentIndex, i32 value) override;
    void clearAttachment(u32 attachmentIndex, f32 value) override;

  private:
    void invalidate();
  };
}