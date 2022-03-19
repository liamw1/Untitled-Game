#pragma once
#include <any>

namespace Engine
{
  enum class FramebufferTextureFormat
  {
    None = 0,

    // Color
    RED_INTEGER,
    RGBA8,

    // Depth/stencil
    DEPTH24STENCIL8,

    // Defaults
    Depth = DEPTH24STENCIL8
  };

  struct FramebufferTextureSpecification
  {
    FramebufferTextureSpecification() = default;
    FramebufferTextureSpecification(FramebufferTextureFormat format)
      : textureFormat(format) {}

    FramebufferTextureFormat textureFormat = FramebufferTextureFormat::None;
    // TODO: Filtering/wrap
  };

  struct FramebufferSpecification
  {
    FramebufferSpecification() = default;
    FramebufferSpecification(const std::initializer_list<FramebufferTextureSpecification>&attachmentList)
      : attachments(attachmentList) {}

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t samples = 1;
    bool swapChainTarget = false;

    std::vector<FramebufferTextureSpecification> attachments;
  };

  class Framebuffer
  {
  public:
    virtual ~Framebuffer() = default;

    virtual void bind() = 0;
    virtual void unbind() = 0;

    virtual const FramebufferSpecification& getSpecification() const = 0;
    virtual uint32_t getColorAttachmentRendererID(uint32_t index = 0) const = 0;

    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual int readPixel(uint32_t attachmentIndex, int x, int y) = 0;

    virtual void clearAttachment(uint32_t attachmentIndex, const std::any& value) = 0;

    static Shared<Framebuffer> Create(const FramebufferSpecification& specification);
  };
}