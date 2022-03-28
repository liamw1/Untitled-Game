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

    const FramebufferSpecification& getSpecification() const override { return m_Specification; }
    uint32_t getColorAttachmentRendererID(uint32_t index = 0) const;

    void invalidate();

    void resize(uint32_t width, uint32_t height) override;
    int readPixel(uint32_t attachmentIndex, int x, int y) override;

    void clearAttachment(uint32_t attachmentIndex, const std::variant<int, float>& value);

  private:
    uint32_t m_RendererID = 0;
    FramebufferSpecification m_Specification;

    std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
    FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::None;

    std::vector<uint32_t> m_ColorAttachments;
    uint32_t m_DepthAttachment = 0;
  };
}